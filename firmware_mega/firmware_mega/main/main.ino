#include <Arduino.h>
#include <Stepper.h>

// Reset-cause diagnostics (AVR Mega2560)
#include <avr/io.h>
#include <avr/wdt.h>

static uint8_t g_mcusrMirror __attribute__((section(".noinit")));

// Runs very early during startup (before setup), captures reset flags and disables WDT.
void captureResetCause() __attribute__((naked)) __attribute__((section(".init3")));
void captureResetCause() {
  g_mcusrMirror = MCUSR;
  MCUSR = 0;
  wdt_disable();
}

/* CORE */
#include "src/core/config.h"
#include "src/core/system_state.h"
#include "src/core/scheduler.h"
#include "src/core/arbitration.h"
#include "src/core/event_bus.h"

/* ACTUATORS */
#include "src/actuators/servo_door.h"
#include "src/actuators/stepper_curtain.h"
#include "src/actuators/buzzer_mgr.h"
#include "src/actuators/status_led.h"
#include "src/actuators/fan_mgr.h"
#include "src/actuators/door_lock.h"

/* SENSORS */
#include "src/sensors/gas_sensor.h"
#include "src/sensors/fire_sensor.h"
#include "src/sensors/ir_sensor.h"
#include "src/sensors/ultrasonic.h"
#include "src/sensors/dht_sensor.h"
#include "src/sensors/door_switch.h"
#include "src/sensors/power_monitor.h"
#include "src/sensors/ldr_sensor.h"

/* INPUT */
#include "src/input/keypad_mgr.h"
#include "src/input/emergency_button.h"

/* EMERGENCY */
#include "src/emergency/emergency_mgr.h"

/* COMM */
#include "src/comm/uart_protocol.h"
#include "src/comm/heartbeat.h"

/* UI */
#include "src/ui/lcd1604_mgr.h"

/* SECURITY */
#include "src/security/anti_bruteforce.h"

/* HEALTH */
#include "src/health/health_monitor.h"
#include "src/health/watchdog.h"

/* ACTUATORS */
FanMgr fanA;
FanMgr fanB;
BuzzerMgr buzzer;
StatusLED statusLED;
ServoDoor servoDoor;
DoorLock doorLock;
StepperCurtain curtain;
static Stepper curtainMotor(2048, PIN_STEPPER_IN1, PIN_STEPPER_IN2, PIN_STEPPER_IN3, PIN_STEPPER_IN4);

/* SENSORS */
GasSensor gasSensor;
FireSensor fireSensor;
IRSensor irSensor;
Ultrasonic ultrasonic;
DHTSensor dht;
DoorSwitch doorSwitch;
PowerMonitor powerMonitor;
LdrSensor ldr;

/* INPUT */
EmergencyButton emergencyButton;

/* EMERGENCY */
EmergencyMgr emergencyMgr;

/* COMM */
UartProtocol uart;
Heartbeat heartbeat;

/* UI */
LCD1604Mgr lcd(0x27);

/* SECURITY */
AntiBruteforce antiBrute;

/* HEALTH */
HealthMonitor health;
Watchdog wdt;
static const int CURTAIN_STEPS_PER_ACTION = 2038 * 6;

// Ultrasonic rules (door safety + access)
static const int DOOR_NEAR_CM = 15;               // <= 15cm: user is considered standing at the door
static const unsigned long DOOR_PRESENCE_MS = 5000UL; // 5s presence alert threshold
static const unsigned long DOOR_BLOCK_BEEP_MS = 5000UL; // 5s low-frequency beeps when door move blocked

enum PendingDoorAction { PEND_NONE, PEND_OPEN, PEND_CLOSE };
static PendingDoorAction g_pendingDoor = PEND_NONE;
static unsigned long g_pendingDoorSinceMs = 0;
static unsigned long g_pendingDoorNextBeepMs = 0;

// Door automation
static unsigned long g_doorAutoCloseAtMs = 0;
static unsigned long g_doorScheduledCloseAtMs = 0;
static bool g_doorHoldOpen = false;
static bool g_prevEmergencyActive = false;

static bool g_presenceActive = false;
static bool g_presenceAlerted = false;
static unsigned long g_presenceSinceMs = 0;

static bool g_doorNear = false;
static bool g_doorFar = false;
static bool g_lastRawDoorNear = false;
static bool g_lastRawDoorFar = false;
static unsigned long g_doorNearLastChangeMs = 0;
static unsigned long g_doorFarLastChangeMs = 0;
static const unsigned long DOOR_NEAR_DEBOUNCE_MS = 150UL;
static const unsigned long DOOR_FAR_DEBOUNCE_MS = 250UL;

static void requestDoorOpen_() {
  if (servoDoor.isOpen() || g_pendingDoor == PEND_OPEN) return;

  const bool safeToMove = SERVO_DIAG_IGNORE_DOOR_SAFETY ? true : g_doorFar;
  if (safeToMove) {
    g_pendingDoor = PEND_NONE;
    g_pendingDoorSinceMs = 0;
    g_pendingDoorNextBeepMs = 0;
    doorLock.unlock();
    servoDoor.open();
    if (DOOR_AUTO_CLOSE_ENABLE && !g_doorHoldOpen) g_doorAutoCloseAtMs = millis() + DOOR_AUTO_CLOSE_MS;
  } else {
    g_pendingDoor = PEND_OPEN;
    g_pendingDoorSinceMs = millis();
    g_pendingDoorNextBeepMs = millis();
  }
}

static void requestDoorClose_() {
  // Any explicit close request should cancel a pending auto-close.
  g_doorAutoCloseAtMs = 0;
  g_doorScheduledCloseAtMs = 0;

  if (!servoDoor.isOpen() || g_pendingDoor == PEND_CLOSE) return;

  const bool safeToMove = SERVO_DIAG_IGNORE_DOOR_SAFETY ? true : g_doorFar;
  if (safeToMove) {
    g_pendingDoor = PEND_NONE;
    g_pendingDoorSinceMs = 0;
    g_pendingDoorNextBeepMs = 0;
    servoDoor.close();
    doorLock.lock();
  } else {
    g_pendingDoor = PEND_CLOSE;
    g_pendingDoorSinceMs = millis();
    g_pendingDoorNextBeepMs = millis();
  }
}


enum UiState { UI_LOCKED, UI_EMERGENCY };
static UiState uiState = UI_LOCKED;

static char inputBuffer[16] = {0};  // Use char array instead of String
static uint8_t inputLength = 0;     // Track length explicitly

enum KeyUiMode {
  KEYMODE_PIN,
  KEYMODE_ADMIN_SEQ,
  KEYMODE_ADMIN_MENU,
  KEYMODE_ADMIN_INPUT
};

static KeyUiMode keyMode = KEYMODE_PIN;

enum AdminWaitMode {
  ADMINWAIT_NONE,
  ADMINWAIT_SEQ,
  ADMINWAIT_OP
};

static AdminWaitMode adminWait = ADMINWAIT_NONE;
static unsigned long adminWaitSinceMs = 0;

enum AdminAction {
  ACT_ADD_PIN,
  ACT_DEL_PIN,
  ACT_ADD_EPIN,
  ACT_DEL_EPIN,
  ACT_SET_MASTER
};

static AdminAction adminAction = ACT_ADD_PIN;
static uint8_t adminMenuIndex = 0;

static const char* const ADMIN_MENU_ITEMS[] = {
  "ADD PIN",
  "DEL PIN",
  "ADD EMG",
  "DEL EMG",
  "CHG PIN",
  "EXIT"
};
static const uint8_t ADMIN_MENU_COUNT = (uint8_t)(sizeof(ADMIN_MENU_ITEMS) / sizeof(ADMIN_MENU_ITEMS[0]));

static void clearInput_() {
  inputLength = 0;
  inputBuffer[0] = 0;
}

static void backspace_() {
  if (inputLength == 0) return;
  inputLength--;
  inputBuffer[inputLength] = 0;
}

static void appendDigit_(char k) {
  if (k < '0' || k > '9') return;
  if (inputLength >= 8) return;
  inputBuffer[inputLength++] = k;
  inputBuffer[inputLength] = 0;
}

static void buildMasked_(char out[17]) {
  for (uint8_t i = 0; i < 16; i++) out[i] = ' ';
  out[16] = 0;
  const uint8_t n = inputLength > 16 ? 16 : inputLength;
  for (uint8_t i = 0; i < n; i++) out[i] = '*';
}

// Hostage/emergency PIN alarm state (commanded by ESP32)
static bool hostageActive = false;
static unsigned long hostageStartMs = 0;
static unsigned long hostageNextBeepMs = 0;
static bool hostageEscalated = false;

static bool manualFanState = false;
static bool rainbowMode = false;
static bool autoFanActive = false;
static bool autoDarkLedActive = false;
// Per-segment manual OFF override: bit0=Light1, bit1=Light2, bit2=Light3.
// If a bit is set, auto-darkness must not turn that segment ON.
static uint8_t manualLedOffMask = 0;
static uint8_t autoDarkMask = 0; // which segments auto-dark turned on
static uint8_t autoDarkColorId = 0; // default WHITE for auto-dark

static float currentTemp = 0.0f;
static float currentHumid = 0.0f;
static unsigned long lastDhtReadMs = 0;

static unsigned long statusUntilMs = 0;
static bool lastIrDetected = false;
static uint8_t lastIrRaw = 255;
static int lastUs = -1;
static bool lastUsOk = false;
static int lastLdr = -1;
static bool lastDark = false;
static bool lastIntrusion = false;

static unsigned long lastTelemetryMs = 0;
static int gasThresholdRuntime = GAS_DANGER_THRESHOLD;

static const __FlashStringHelper* resetFlagName(uint8_t mcusr) {
  if (mcusr & _BV(WDRF)) return F("WDT");
  if (mcusr & _BV(BORF)) return F("BOR");
  if (mcusr & _BV(EXTRF)) return F("EXT");
  if (mcusr & _BV(PORF)) return F("POR");
  return F("UNK");
}

void setup() {
  // Match your working sketch baudrate
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println(F("=== SMART HOME SYSTEM STARTING ==="));

  // Report last reset cause (helps distinguish WDT vs brownout vs external reset)
  Serial.print(F("ResetFlags="));
  Serial.print(g_mcusrMirror, HEX);
  Serial.print(F(" Cause="));
  Serial.println(resetFlagName(g_mcusrMirror));

  g_systemState.init();
  eventBusInit();
  schedulerInit();

  fanA.begin(PIN_RELAY_FAN, FAN_RELAY_ACTIVE_LOW);
  fanB.begin(PIN_RELAY_SOCKET, FAN_RELAY_ACTIVE_LOW);
  buzzer.begin(PIN_BUZZER, BUZZER_ACTIVE_LOW, BUZZER_USE_TONE, BUZZER_TONE_HZ);
  
  // Limit LED brightness to prevent brownout
  statusLED.begin(PIN_STATUS_LED, STATUS_LED_COUNT);
  
  servoDoor.begin(PIN_SERVO_DOOR, SERVO_DOOR_OPEN_ANGLE, SERVO_DOOR_CLOSE_ANGLE);
  servoDoor.enablePowerSaving(true, SERVO_DETACH_AFTER_MS);
  
  doorLock.begin(PIN_DOOR_LOCK, true);

  curtainMotor.setSpeed(10);
  curtain.begin(&curtainMotor, CURTAIN_STEPS_PER_ACTION);

  // Gas calibration (prevents false stuck emergency while keeping gas detection active)
  {
    unsigned long sum = 0;
    for (int i = 0; i < GAS_CALIBRATION_SAMPLES; i++) {
      sum += (unsigned long)analogRead(PIN_GAS_SENSOR);
      delay(GAS_CALIBRATION_DELAY_MS);
    }
    const int baseline = (int)(sum / (unsigned long)GAS_CALIBRATION_SAMPLES);
    gasThresholdRuntime = baseline + GAS_CALIBRATION_MARGIN;
    if (gasThresholdRuntime > 1023) gasThresholdRuntime = 1023;
    Serial.print(F("Gas baseline=")); Serial.print(baseline);
    Serial.print(F(" threshold=")); Serial.println(gasThresholdRuntime);
  }

  gasSensor.begin(PIN_GAS_SENSOR, gasThresholdRuntime);
  fireSensor.begin(PIN_FIRE_SENSOR);
  irSensor.begin(PIN_IR_SENSOR, IR_DETECTED_LEVEL, IR_USE_PULLUP);
  ultrasonic.begin(PIN_ULTRASONIC_TRIG, PIN_ULTRASONIC_ECHO, ULTRASONIC_TIMEOUT_US);
  dht.begin(PIN_DHT_SENSOR, DHT_TYPE);
  doorSwitch.begin(PIN_DOOR_SWITCH);
  powerMonitor.begin(PIN_POWER_MONITOR);
  ldr.begin(PIN_LDR_SENSOR, LDR_THRESHOLD, LDR_DARK_WHEN_HIGH);

  keypadInit();
  emergencyButton.begin(PIN_EMERGENCY_BUTTON, true);

  emergencyMgr.begin();
  emergencyMgr.attachSensors(&gasSensor, &fireSensor, &irSensor);
  emergencyMgr.attachActuators(&fanA, &fanB, &buzzer, &servoDoor, &doorLock);

  uart.begin(Serial1);
  heartbeat.begin(1000);

  // Also send reset cause to ESP32
  uart.sendKV("RSTF", (int)g_mcusrMirror);
  // Send short name without String allocations
  if (g_mcusrMirror & _BV(WDRF)) uart.sendKV("RST", "WDT");
  else if (g_mcusrMirror & _BV(BORF)) uart.sendKV("RST", "BOR");
  else if (g_mcusrMirror & _BV(EXTRF)) uart.sendKV("RST", "EXT");
  else if (g_mcusrMirror & _BV(PORF)) uart.sendKV("RST", "POR");
  else uart.sendKV("RST", "UNK");

  lcd.begin();
  lcd.showStatus2("SYSTEM INIT", "Please wait...");
  delayMicroseconds(100);  // Minimal delay instead of blocking delay(1000)
  
  lcd.showStatus2("SYSTEM READY", "Waiting...");
  delayMicroseconds(50);  // Minimal delay

  // Quick hardware confirmation: short beep on boot
  buzzer.beep(80);

  health.begin();
  wdt.begin();
  
  Serial.println(F("=== SYSTEM READY ==="));
  Serial.print(F("Free RAM: ")); Serial.println(health.getFreeRAM());
}

void loop() {
  wdt.feed();  // Critical: feed watchdog at start

  gasSensor.update();
  wdt.feed();
  fireSensor.update();
  wdt.feed();
  irSensor.update();
  wdt.feed();
  ultrasonic.update();
  wdt.feed();
  ldr.update();
  wdt.feed();
  dht.update();
  wdt.feed();
  emergencyButton.update();
  keypadUpdate();
  wdt.feed();

  emergencyMgr.update();
  wdt.feed();

  const bool emergencyActive = emergencyMgr.active() || emergencyButton.pressed();
  if (emergencyActive) {
    // Keep emergency flow simple: do not auto-close while emergency is active.
    g_doorAutoCloseAtMs = 0;
    g_doorScheduledCloseAtMs = 0;
    if (g_pendingDoor == PEND_CLOSE) {
      g_pendingDoor = PEND_NONE;
      g_pendingDoorSinceMs = 0;
      g_pendingDoorNextBeepMs = 0;
    }
    uiState = UI_EMERGENCY;
    g_systemState.setMode(MODE_EMERGENCY);
    arbitrationSetPriority(PRIORITY_EMERGENCY);
    
    // Show emergency status
    static unsigned long lastEmergencyUpdate = 0;
    if (millis() - lastEmergencyUpdate > 500) {
      lastEmergencyUpdate = millis();
      // Requirement: do not show sensor details on LCD
      lcd.showEmergency("EMERGENCY!");
      wdt.feed();  // Feed after LCD to prevent timeout
    }
    
    statusLED.setEmergency(true);
  } else if (uiState == UI_EMERGENCY) {
    // Return to locked after emergency clears
    uiState = UI_LOCKED;
    clearInput_();
    keyMode = KEYMODE_PIN;
    adminWait = ADMINWAIT_NONE;
    // NOTE: DO NOT TURN OFF LED HERE - let statusLED handle fade out
    // statusLED.setEmergency(false);  // Removed - let fade effect complete
    lcd.showStatus2("SAFE", "Waiting...");
    wdt.feed();

    // When emergency clears, wait for ESP32 voice to finish, then close+lock.
    if (DOOR_CLOSE_AFTER_EMERGENCY && !g_doorHoldOpen) {
      g_doorAutoCloseAtMs = 0;
      g_doorScheduledCloseAtMs = millis() + DOOR_CLOSE_AFTER_EMERGENCY_DELAY_MS;
    }


  // Track emergency edge (for safety if UI state changes elsewhere)
  g_prevEmergencyActive = emergencyActive;
  }

  // Keep LED manager informed even if UI state doesn't transition
  // Requirement: hostage/emergency-PIN LED only blinks when siren is sounding.
  if (!emergencyActive && uiState != UI_EMERGENCY) {
    statusLED.setEmergency(hostageEscalated);
  }

  if (!emergencyActive) {
    g_systemState.setMode(MODE_AUTO);
    arbitrationSetPriority(PRIORITY_AUTO);

    // Read DHT every 3 seconds (was 2s, reduced load)
    if (millis() - lastDhtReadMs >= 3000UL) {
      lastDhtReadMs = millis();
      float t = dht.temperature();
      float h = dht.humidity();
      if (!isnan(t)) currentTemp = t;
      if (!isnan(h)) currentHumid = h;
    }

    // AUTO MODE BEHAVIORS:
    // 1. Temperature-based fan control (with hysteresis)
    if (currentTemp >= TEMP_AUTO_FAN_ON && !autoFanActive) {
      autoFanActive = true;
      fanA.set(true);
      fanB.set(true);
    } else if (currentTemp <= TEMP_AUTO_FAN_OFF && autoFanActive) {
      autoFanActive = false;
      fanA.set(false);
      fanB.set(false);
    }

    // 2. Intrusion detection with edge beep
    const int usCm = ultrasonic.distanceCm();
    const bool usOk = ultrasonic.ok();
    const bool ir = irSensor.motionDetected();
    const uint8_t irRaw = irSensor.raw();
    const int ldrRaw = ldr.value();
    const bool dark = ldr.isDark();

    // Debounce ultrasonic threshold around 15cm for stable logic.
    {
      const unsigned long now = millis();
      const bool rawNear = (usOk && usCm > 0 && usCm <= DOOR_NEAR_CM);
      const bool rawFar = (usOk && usCm > DOOR_NEAR_CM);

      if (rawNear != g_lastRawDoorNear) {
        g_lastRawDoorNear = rawNear;
        g_doorNearLastChangeMs = now;
      }
      if (rawFar != g_lastRawDoorFar) {
        g_lastRawDoorFar = rawFar;
        g_doorFarLastChangeMs = now;
      }

      if (rawNear != g_doorNear && (now - g_doorNearLastChangeMs) >= DOOR_NEAR_DEBOUNCE_MS) {
        g_doorNear = rawNear;
      }
      if (rawFar != g_doorFar && (now - g_doorFarLastChangeMs) >= DOOR_FAR_DEBOUNCE_MS) {
        g_doorFar = rawFar;
      }
    }

    // IR alone OR (ultrasonic valid AND close)
    const bool rawIntrusion = ir || (usOk && (usCm <= INTRUSION_DISTANCE_CM));

    // Debounce intrusion to avoid random beeps on noisy sensors
    static bool debouncedIntrusion = false;
    static bool lastRawIntrusion = false;
    static unsigned long intrusionLastChangeMs = 0;
    const unsigned long nowMs = millis();
    static const unsigned long INTRUSION_ON_DEBOUNCE_MS = 250UL;
    static const unsigned long INTRUSION_OFF_DEBOUNCE_MS = 600UL;

    if (rawIntrusion != lastRawIntrusion) {
      lastRawIntrusion = rawIntrusion;
      intrusionLastChangeMs = nowMs;
    }

    const unsigned long debounceMs = lastRawIntrusion ? INTRUSION_ON_DEBOUNCE_MS
                                                      : INTRUSION_OFF_DEBOUNCE_MS;
    if ((nowMs - intrusionLastChangeMs) >= debounceMs) {
      debouncedIntrusion = lastRawIntrusion;
    }

    // 3. Auto LED darkness mode: when dark -> turn LEDs ON (unless user manually forced them OFF).
    // Auto mode only engages when user hasn't manually set LED segments (same as before).
    if (!rainbowMode && (!statusLED.anySegmentTarget() || autoDarkLedActive)) {
      if (dark && !autoDarkLedActive) {
        const uint8_t desired = (uint8_t)(0x07 & (uint8_t)(~manualLedOffMask));
        if (desired != 0) {
          autoDarkLedActive = true;
          autoDarkMask = desired;
          statusLED.setRainbowTarget(false);
          statusLED.setAllSegmentsColorId(autoDarkColorId);
          // Apply per-segment targets
          statusLED.setSegmentTarget(0, (autoDarkMask & 0x01) != 0);
          statusLED.setSegmentTarget(1, (autoDarkMask & 0x02) != 0);
          statusLED.setSegmentTarget(2, (autoDarkMask & 0x04) != 0);
        } else {
          autoDarkLedActive = false;
          autoDarkMask = 0;
        }
      } else if (!dark && autoDarkLedActive) {
        autoDarkLedActive = false;
        // Turn off only segments that auto-dark turned on
        statusLED.setSegmentTarget(0, false);
        statusLED.setSegmentTarget(1, false);
        statusLED.setSegmentTarget(2, false);
        autoDarkMask = 0;
      }
    }

    // 4. Sensor change detection & notification
    const bool sensorChanged =
      (ir != lastIrDetected) || (irRaw != lastIrRaw) ||
      (usCm != lastUs) || (usOk != lastUsOk) ||
      (ldrRaw != lastLdr) || (dark != lastDark) ||
      (debouncedIntrusion != lastIntrusion);

    if (sensorChanged) {
      lastIrDetected = ir;
      lastIrRaw = irRaw;
      lastUs = usCm;
      lastUsOk = usOk;
      lastLdr = ldrRaw;
      lastDark = dark;

      // Beep only on intrusion state change
      if (debouncedIntrusion != lastIntrusion) {
        buzzer.beep(debouncedIntrusion ? 150 : 60);
        lastIntrusion = debouncedIntrusion;
      }

      // Requirement: do not show sensor info on LCD
    }

    // 5. Door presence detection (<=15cm for >DOOR_PRESENCE_MS while door not opened)
    {
      const bool doorOpenNow = servoDoor.isOpen();
      const bool nearDoor = g_doorNear;

      if (nearDoor && !doorOpenNow) {
        if (!g_presenceActive) {
          g_presenceActive = true;
          g_presenceAlerted = false;
          g_presenceSinceMs = millis();
        } else if (!g_presenceAlerted && (millis() - g_presenceSinceMs) >= DOOR_PRESENCE_MS) {
          g_presenceAlerted = true;
          // Local buzzer alert. Voice will be played by ESP32 when it receives PRES=1.
          buzzer.beep(250);
        }
      } else {
        g_presenceActive = false;
        g_presenceAlerted = false;
        g_presenceSinceMs = 0;
      }
    }

    // Keypad: PIN entry + admin menu (*<PIN>#)
    if (millis() >= statusUntilMs) {
      char key = keypadReadKey();
      if (key) {
        // If waiting for admin ack, ignore keys.
        if (adminWait != ADMINWAIT_NONE) {
          // ignore
        } else if (keyMode == KEYMODE_PIN) {
          if (key >= '0' && key <= '9') {
            appendDigit_(key);
          } else if (key == '*') {
            if (inputLength == 0) {
              keyMode = KEYMODE_ADMIN_SEQ;
              clearInput_();
              lcd.showStatus2("ADMIN", "Nhap PIN + #");
            } else {
              backspace_();
            }
          } else if (key == '#') {
            if (inputLength > 0) {
              // Requirement: user must stand close (<=15cm) to enter PIN.
              if (!(usOk && usCm > 0 && usCm <= DOOR_NEAR_CM)) {
                buzzer.beep(120);
                lcd.showStatus2("CANH BAO", "Dung gan cua");
                statusUntilMs = millis() + 1200UL;
                // Keep the typed PIN so user can retry when standing closer.
              } else {
                char cmd[32];
                snprintf(cmd, sizeof(cmd), "DOOR_PIN:%s", inputBuffer);
                uart.sendCmd(cmd);
                clearInput_();
                lcd.showStatus2("MAT KHAU", "Dang kiem tra");
                statusUntilMs = millis() + 1500UL;
              }
            }
          }
        } else if (keyMode == KEYMODE_ADMIN_SEQ) {
          if (key >= '0' && key <= '9') {
            appendDigit_(key);
          } else if (key == '*') {
            if (inputLength == 0) {
              keyMode = KEYMODE_PIN;
            } else {
              backspace_();
            }
          } else if (key == '#') {
            if (inputLength > 0) {
              char cmd[40];
              snprintf(cmd, sizeof(cmd), "ADMIN_SEQ:%s", inputBuffer);
              uart.sendCmd(cmd);
              clearInput_();
              adminWait = ADMINWAIT_SEQ;
              adminWaitSinceMs = millis();
              lcd.showStatus2("ADMIN", "Dang kiem tra");
              statusUntilMs = millis() + 1500UL;
            } else {
              keyMode = KEYMODE_PIN;
            }
          }
        } else if (keyMode == KEYMODE_ADMIN_MENU) {
          if (key == '2') {
            adminMenuIndex = (uint8_t)((adminMenuIndex + 1) % ADMIN_MENU_COUNT);
          } else if (key == '8') {
            adminMenuIndex = (uint8_t)((adminMenuIndex + ADMIN_MENU_COUNT - 1) % ADMIN_MENU_COUNT);
          } else if (key == 'D') {
            keyMode = KEYMODE_PIN;
            clearInput_();
          } else if (key == '#') {
            if (adminMenuIndex == 5) {
              keyMode = KEYMODE_PIN;
              clearInput_();
            } else {
              if (adminMenuIndex == 0) adminAction = ACT_ADD_PIN;
              else if (adminMenuIndex == 1) adminAction = ACT_DEL_PIN;
              else if (adminMenuIndex == 2) adminAction = ACT_ADD_EPIN;
              else if (adminMenuIndex == 3) adminAction = ACT_DEL_EPIN;
              else adminAction = ACT_SET_MASTER;
              keyMode = KEYMODE_ADMIN_INPUT;
              clearInput_();
            }
          }
        } else if (keyMode == KEYMODE_ADMIN_INPUT) {
          if (key >= '0' && key <= '9') {
            appendDigit_(key);
          } else if (key == '*') {
            if (inputLength == 0) {
              keyMode = KEYMODE_ADMIN_MENU;
            } else {
              backspace_();
            }
          } else if (key == '#') {
            if (inputLength > 0) {
              char cmd[48];
              const char* prefix = "";
              switch (adminAction) {
                case ACT_ADD_PIN: prefix = "PIN_ADD:"; break;
                case ACT_DEL_PIN: prefix = "PIN_DEL:"; break;
                case ACT_ADD_EPIN: prefix = "EPIN_ADD:"; break;
                case ACT_DEL_EPIN: prefix = "EPIN_DEL:"; break;
                case ACT_SET_MASTER: prefix = "MASTER_PIN_SET:"; break;
              }
              snprintf(cmd, sizeof(cmd), "%s%s", prefix, inputBuffer);
              uart.sendCmd(cmd);
              clearInput_();
              adminWait = ADMINWAIT_OP;
              adminWaitSinceMs = millis();
              lcd.showStatus2("ADMIN", "Dang xu ly");
              statusUntilMs = millis() + 1500UL;
            }
          }
        }
      }

      // Render prompt when no overlay
      if (millis() >= statusUntilMs) {
        if (keyMode == KEYMODE_PIN) {
          char masked[17];
          buildMasked_(masked);
          lcd.showLocked(masked);
        } else if (keyMode == KEYMODE_ADMIN_SEQ) {
          char masked[17];
          buildMasked_(masked);
          lcd.showStatus2("ADMIN PIN:", masked);
        } else if (keyMode == KEYMODE_ADMIN_MENU) {
          lcd.showMenuNav(ADMIN_MENU_ITEMS[adminMenuIndex]);
        } else if (keyMode == KEYMODE_ADMIN_INPUT) {
          char masked[17];
          buildMasked_(masked);
          switch (adminAction) {
            case ACT_ADD_PIN: lcd.showStatus2("ADD PIN:", masked); break;
            case ACT_DEL_PIN: lcd.showStatus2("DEL PIN:", masked); break;
            case ACT_ADD_EPIN: lcd.showStatus2("ADD EMG:", masked); break;
            case ACT_DEL_EPIN: lcd.showStatus2("DEL EMG:", masked); break;
            case ACT_SET_MASTER: lcd.showStatus2("CHG PIN:", masked); break;
          }
        }
      }

      // Simple timeout for admin waits
      if (adminWait != ADMINWAIT_NONE && (millis() - adminWaitSinceMs) > 3000UL) {
        adminWait = ADMINWAIT_NONE;
        keyMode = KEYMODE_PIN;
        clearInput_();
        lcd.showStatus2("ADMIN", "Timeout");
        statusUntilMs = millis() + 1200UL;
      }
    }
  }

  // Update actuators
  // Door safety gate: open/close only when user stands farther than 15cm.
  // If blocked, beep low-frequency and keep retrying until condition is met.
  {
    if (g_pendingDoor != PEND_NONE) {
      const bool safeToMove = SERVO_DIAG_IGNORE_DOOR_SAFETY ? true : g_doorFar;

      if (safeToMove) {
        if (g_pendingDoor == PEND_OPEN) {
          doorLock.unlock();
          servoDoor.open();
          if (DOOR_AUTO_CLOSE_ENABLE && !g_doorHoldOpen) g_doorAutoCloseAtMs = millis() + DOOR_AUTO_CLOSE_MS;
        } else if (g_pendingDoor == PEND_CLOSE) {
          servoDoor.close();
          doorLock.lock();
        }
        g_pendingDoor = PEND_NONE;
        g_pendingDoorSinceMs = 0;
        g_pendingDoorNextBeepMs = 0;
      } else {
        const unsigned long now = millis();
        // Beep pattern:
        // - first DOOR_BLOCK_BEEP_MS: beep every 700ms
        // - after that: beep every 2000ms until safe
        const unsigned long elapsed = (g_pendingDoorSinceMs == 0) ? 0 : (now - g_pendingDoorSinceMs);
        const unsigned long interval = (elapsed < DOOR_BLOCK_BEEP_MS) ? 700UL : 2000UL;
        if ((long)(now - g_pendingDoorNextBeepMs) >= 0) {
          buzzer.beep(60);
          g_pendingDoorNextBeepMs = now + interval;
        }
      }
    }
  }

  // Auto-close door after it has been opened for a while.
  if (!emergencyActive && !g_doorHoldOpen && DOOR_AUTO_CLOSE_ENABLE && servoDoor.isOpen() && g_doorAutoCloseAtMs != 0) {
    if ((long)(millis() - g_doorAutoCloseAtMs) >= 0) {
      requestDoorClose_();
    }
  }

  // Close after voice finishes (scheduled by emergency clear / hostage clear).
  if (!emergencyActive && !g_doorHoldOpen && g_doorScheduledCloseAtMs != 0) {
    if ((long)(millis() - g_doorScheduledCloseAtMs) >= 0) {
      g_doorScheduledCloseAtMs = 0;
      requestDoorClose_();
    }
  }

  servoDoor.update();  // Handle servo auto-detach
  wdt.feed();

  uart.update();
  wdt.feed();
  if (uart.hasCommand()) {
    UartCommand cmd = uart.getCommand();
    const char* arg = uart.getArg();
    g_systemState.setMode(MODE_MANUAL);
    arbitrationSetPriority(PRIORITY_MANUAL);

    // Manual/web priority per light:
    // - OFF for a segment sets its bit in manualLedOffMask
    // - ON/color for a segment clears its bit
    // - ALL/Rainbow clears all bits
    switch (cmd) {
      case CMD_LED_OFF:
        manualLedOffMask = 0x07;
        break;
      case CMD_SEG1_OFF:
        manualLedOffMask |= 0x01;
        break;
      case CMD_SEG2_OFF:
        manualLedOffMask |= 0x02;
        break;
      case CMD_SEG3_OFF:
        manualLedOffMask |= 0x04;
        break;

      case CMD_SEG1_ON:
      case CMD_SEG1_WHITE:
      case CMD_SEG1_RED:
      case CMD_SEG1_GREEN:
      case CMD_SEG1_BLUE:
      case CMD_SEG1_YELLOW:
      case CMD_SEG1_PURPLE:
        manualLedOffMask &= (uint8_t)~0x01;
        break;

      case CMD_SEG2_ON:
      case CMD_SEG2_WHITE:
      case CMD_SEG2_RED:
      case CMD_SEG2_GREEN:
      case CMD_SEG2_BLUE:
      case CMD_SEG2_YELLOW:
      case CMD_SEG2_PURPLE:
        manualLedOffMask &= (uint8_t)~0x02;
        break;

      case CMD_SEG3_ON:
      case CMD_SEG3_WHITE:
      case CMD_SEG3_RED:
      case CMD_SEG3_GREEN:
      case CMD_SEG3_BLUE:
      case CMD_SEG3_YELLOW:
      case CMD_SEG3_PURPLE:
        manualLedOffMask &= (uint8_t)~0x04;
        break;

      case CMD_LED_ALL:
      case CMD_LED_RAINBOW:
      case CMD_ALL_WHITE:
      case CMD_ALL_RED:
      case CMD_ALL_GREEN:
      case CMD_ALL_BLUE:
      case CMD_ALL_YELLOW:
      case CMD_ALL_PURPLE:
        manualLedOffMask = 0;
        break;
      default:
        break;
    }

    switch (cmd) {
      case CMD_PIN_OK:
        lcd.showStatus2("MAT KHAU", "Thanh cong");
        buzzer.beep(60);
        statusUntilMs = millis() + 1500UL;
        if (!emergencyActive && DOOR_AUTO_OPEN_ON_AUTH) requestDoorOpen_();
        break;
      case CMD_PIN_FAIL:
        lcd.showStatus2("MAT KHAU", "Sai");
        buzzer.beep(180);
        statusUntilMs = millis() + 1500UL;
        break;

      case CMD_ADMIN_OK:
        if (adminWait == ADMINWAIT_SEQ) {
          adminWait = ADMINWAIT_NONE;
          keyMode = KEYMODE_ADMIN_MENU;
          adminMenuIndex = 0;
          lcd.showMenuNav(ADMIN_MENU_ITEMS[adminMenuIndex]);
        } else if (adminWait == ADMINWAIT_OP) {
          adminWait = ADMINWAIT_NONE;
          lcd.showStatus2("ADMIN", "OK");
          statusUntilMs = millis() + 1200UL;
        } else {
          lcd.showStatus2("ADMIN", "OK");
          statusUntilMs = millis() + 1200UL;
        }
        buzzer.beep(60);
        break;
      case CMD_ADMIN_FAIL:
        if (adminWait == ADMINWAIT_SEQ) {
          adminWait = ADMINWAIT_NONE;
          keyMode = KEYMODE_PIN;
          clearInput_();
          lcd.showStatus2("ADMIN", "Sai");
          statusUntilMs = millis() + 1500UL;
        } else if (adminWait == ADMINWAIT_OP) {
          adminWait = ADMINWAIT_NONE;
          lcd.showStatus2("ADMIN", "That bai");
          statusUntilMs = millis() + 1500UL;
        } else {
          lcd.showStatus2("ADMIN", "That bai");
          statusUntilMs = millis() + 1500UL;
        }
        buzzer.beep(180);
        break;

      case CMD_RFID_CHECK:
        lcd.showStatus2("RFID", "Dang kiem tra");
        buzzer.beep(40);
        statusUntilMs = millis() + 2000UL;
        break;
      case CMD_RFID_FAIL:
        lcd.showStatus2("RFID", "Khong thanh cong");
        buzzer.beep(180);
        statusUntilMs = millis() + 2000UL;
        break;
      case CMD_RFID_HELLO: {
        // Line0 fixed, line1 is name (trimmed by LCD manager)
        lcd.showStatus2("Xin chao", arg && arg[0] ? arg : "Ban");
        buzzer.beep(80);
        statusUntilMs = millis() + 2500UL;
        if (!emergencyActive && DOOR_AUTO_OPEN_ON_AUTH) requestDoorOpen_();
        break;
      }

      case CMD_HOSTAGE_ON:
        hostageActive = true;
        hostageStartMs = millis();
        hostageNextBeepMs = hostageStartMs + 5000UL;
        hostageEscalated = false;
        lcd.showStatus2("CANH BAO", "Bi de doa!");
        // Hostage mode: DO NOT force door open/hold-open.
        // Keep door behavior normal (auto-close still works).
        g_doorScheduledCloseAtMs = 0;
        break;
      case CMD_HOSTAGE_BEEP:
        // short, low-volume beep request from ESP32 (we approximate by short beep)
        buzzer.beep(60);
        break;
      case CMD_HOSTAGE_SIREN:
        hostageEscalated = true;
        buzzer.alarm();
        lcd.showStatus2("CANH BAO", "GOI CSGT!");
        break;
      case CMD_HOSTAGE_CLEAR:
        hostageActive = false;
        hostageStartMs = 0;
        hostageNextBeepMs = 0;
        hostageEscalated = false;
        buzzer.off();
        buzzer.beep(80);
        lcd.showStatus2("SAFE", "Waiting...");
        g_doorHoldOpen = false;
        g_doorAutoCloseAtMs = 0;
        // After ESP32 voice finishes, close+lock.
        if (!emergencyActive && DOOR_CLOSE_ON_HOSTAGE_CLEAR) {
          g_doorScheduledCloseAtMs = millis() + DOOR_CLOSE_ON_HOSTAGE_CLEAR_DELAY_MS;
        }
        break;

      case CMD_FAN1_ON: fanA.set(true); break;
      case CMD_FAN1_OFF: fanA.set(false); break;
      case CMD_FAN2_ON: fanB.set(true); break;
      case CMD_FAN2_OFF: fanB.set(false); break;
      case CMD_DOOR_OPEN: {
        requestDoorOpen_();
        break;
      }
      case CMD_DOOR_CLOSE: {
        requestDoorClose_();
        break;
      }
      case CMD_LOCK: doorLock.lock(); break;
      case CMD_UNLOCK: doorLock.unlock(); break;
      case CMD_CURTAIN_OPEN: curtain.open(); break;
      case CMD_CURTAIN_CLOSE: curtain.close(); break;
      case CMD_CURTAIN_STOP: curtain.stop(); break;
      case CMD_SEG1_ON:
        autoDarkLedActive = false;
        autoDarkMask = 0;
        statusLED.setRainbowTarget(false);
        statusLED.setSegmentColorId(0, 0);
        statusLED.setSegmentTarget(0, true);
        rainbowMode = false;
        break;
      case CMD_SEG1_OFF:
        autoDarkLedActive = false;
        autoDarkMask = 0;
        statusLED.setRainbowTarget(false);
        statusLED.setSegmentTarget(0, false);
        rainbowMode = false;
        break;
      case CMD_SEG2_ON:
        autoDarkLedActive = false;
        autoDarkMask = 0;
        statusLED.setRainbowTarget(false);
        statusLED.setSegmentColorId(1, 0);
        statusLED.setSegmentTarget(1, true);
        rainbowMode = false;
        break;
      case CMD_SEG2_OFF:
        autoDarkLedActive = false;
        autoDarkMask = 0;
        statusLED.setRainbowTarget(false);
        statusLED.setSegmentTarget(1, false);
        rainbowMode = false;
        break;
      case CMD_SEG3_ON:
        autoDarkLedActive = false;
        autoDarkMask = 0;
        statusLED.setRainbowTarget(false);
        statusLED.setSegmentColorId(2, 0);
        statusLED.setSegmentTarget(2, true);
        rainbowMode = false;
        break;
      case CMD_SEG3_OFF:
        autoDarkLedActive = false;
        autoDarkMask = 0;
        statusLED.setRainbowTarget(false);
        statusLED.setSegmentTarget(2, false);
        rainbowMode = false;
        break;
      case CMD_SEG1_WHITE:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 0;
        statusLED.setSegmentColorId(0, 0);
        statusLED.setSegmentTarget(0, true);
        rainbowMode = false;
        break;
      case CMD_SEG1_RED:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 1;
        statusLED.setSegmentColorId(0, 1);
        statusLED.setSegmentTarget(0, true);
        rainbowMode = false;
        break;
      case CMD_SEG1_GREEN:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 2;
        statusLED.setSegmentColorId(0, 2);
        statusLED.setSegmentTarget(0, true);
        rainbowMode = false;
        break;
      case CMD_SEG1_BLUE:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 3;
        statusLED.setSegmentColorId(0, 3);
        statusLED.setSegmentTarget(0, true);
        rainbowMode = false;
        break;
      case CMD_SEG1_YELLOW:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 4;
        statusLED.setSegmentColorId(0, 4);
        statusLED.setSegmentTarget(0, true);
        rainbowMode = false;
        break;
      case CMD_SEG1_PURPLE:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 5;
        statusLED.setSegmentColorId(0, 5);
        statusLED.setSegmentTarget(0, true);
        rainbowMode = false;
        break;
      case CMD_SEG2_WHITE:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 0;
        statusLED.setSegmentColorId(1, 0);
        statusLED.setSegmentTarget(1, true);
        rainbowMode = false;
        break;
      case CMD_SEG2_RED:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 1;
        statusLED.setSegmentColorId(1, 1);
        statusLED.setSegmentTarget(1, true);
        rainbowMode = false;
        break;
      case CMD_SEG2_GREEN:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 2;
        statusLED.setSegmentColorId(1, 2);
        statusLED.setSegmentTarget(1, true);
        rainbowMode = false;
        break;
      case CMD_SEG2_BLUE:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 3;
        statusLED.setSegmentColorId(1, 3);
        statusLED.setSegmentTarget(1, true);
        rainbowMode = false;
        break;
      case CMD_SEG2_YELLOW:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 4;
        statusLED.setSegmentColorId(1, 4);
        statusLED.setSegmentTarget(1, true);
        rainbowMode = false;
        break;
      case CMD_SEG2_PURPLE:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 5;
        statusLED.setSegmentColorId(1, 5);
        statusLED.setSegmentTarget(1, true);
        rainbowMode = false;
        break;
      case CMD_SEG3_WHITE:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 0;
        statusLED.setSegmentColorId(2, 0);
        statusLED.setSegmentTarget(2, true);
        rainbowMode = false;
        break;
      case CMD_SEG3_RED:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 1;
        statusLED.setSegmentColorId(2, 1);
        statusLED.setSegmentTarget(2, true);
        rainbowMode = false;
        break;
      case CMD_SEG3_GREEN:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 2;
        statusLED.setSegmentColorId(2, 2);
        statusLED.setSegmentTarget(2, true);
        rainbowMode = false;
        break;
      case CMD_SEG3_BLUE:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 3;
        statusLED.setSegmentColorId(2, 3);
        statusLED.setSegmentTarget(2, true);
        rainbowMode = false;
        break;
      case CMD_SEG3_YELLOW:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 4;
        statusLED.setSegmentColorId(2, 4);
        statusLED.setSegmentTarget(2, true);
        rainbowMode = false;
        break;
      case CMD_SEG3_PURPLE:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 5;
        statusLED.setSegmentColorId(2, 5);
        statusLED.setSegmentTarget(2, true);
        rainbowMode = false;
        break;
      case CMD_ALL_WHITE:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 0;
        statusLED.setAllSegmentsColorId(0);
        statusLED.setAllSegmentsTarget(true);
        rainbowMode = false;
        break;
      case CMD_ALL_RED:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 1;
        statusLED.setAllSegmentsColorId(1);
        statusLED.setAllSegmentsTarget(true);
        rainbowMode = false;
        break;
      case CMD_ALL_GREEN:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 2;
        statusLED.setAllSegmentsColorId(2);
        statusLED.setAllSegmentsTarget(true);
        rainbowMode = false;
        break;
      case CMD_ALL_BLUE:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 3;
        statusLED.setAllSegmentsColorId(3);
        statusLED.setAllSegmentsTarget(true);
        rainbowMode = false;
        break;
      case CMD_ALL_YELLOW:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 4;
        statusLED.setAllSegmentsColorId(4);
        statusLED.setAllSegmentsTarget(true);
        rainbowMode = false;
        break;
      case CMD_ALL_PURPLE:
        statusLED.setRainbowTarget(false);
        autoDarkColorId = 5;
        statusLED.setAllSegmentsColorId(5);
        statusLED.setAllSegmentsTarget(true);
        rainbowMode = false;
        break;
      case CMD_LED_OFF:
        autoDarkLedActive = false;
        autoDarkMask = 0;
        statusLED.allOff();
        rainbowMode = false;
        break;
      case CMD_LED_RAINBOW:
        autoDarkLedActive = false;
        autoDarkMask = 0;
        statusLED.setRainbowTarget(true);
        rainbowMode = true;
        break;
      case CMD_LED_ALL:
        autoDarkLedActive = false;
        autoDarkMask = 0;
        statusLED.setAllSegmentsTarget(true);
        rainbowMode = false;
        break;
      default: break;
    }
  }

  // Hostage local timing (backup): if active and not escalated, beep every 5s; after 20s, siren.
  if (hostageActive) {
    const unsigned long now = millis();
    const unsigned long elapsed = now - hostageStartMs;
    if (elapsed < 20000UL) {
      if ((long)(now - hostageNextBeepMs) >= 0) {
        buzzer.beep(60);
        hostageNextBeepMs += 5000UL;
      }
    } else if (!hostageEscalated) {
      hostageEscalated = true;
      buzzer.alarm();
      lcd.showStatus2("CANH BAO", "GOI CSGT!");
    }
  }

  curtain.update();
  wdt.feed();
  buzzer.update();
  wdt.feed();
  statusLED.update();  // This can take time with NeoPixel
  wdt.feed();

  heartbeat.update();
  health.update();
  wdt.feed();

  // Telemetry for ESP32 (1Hz)
  if (millis() - lastTelemetryMs >= 1000UL) {
    lastTelemetryMs = millis();
    uart.sendKV("MODE", emergencyActive ? "EMG" : "AUTO");
    uart.sendKV("EMG", emergencyActive ? 1 : 0);
    uart.sendKV("UP", (unsigned long)(millis() / 1000UL));
    uart.sendKV("RAM", health.getFreeRAM());
    uart.sendKV("GAS", gasSensor.value());
    uart.sendKV("GTH", gasThresholdRuntime);
    uart.sendKV("FIRE", fireSensor.detected() ? 1 : 0);
    uart.sendKV("IR", irSensor.motionDetected() ? 1 : 0);
    uart.sendKV("US", ultrasonic.ok() ? ultrasonic.distanceCm() : -1);
    uart.sendKV("USOK", ultrasonic.ok() ? 1 : 0);
    uart.sendKV("LDR", ldr.value());
    uart.sendKV("DARK", ldr.isDark() ? 1 : 0);
    uart.sendKV("LTH", (int)LDR_THRESHOLD);
    uart.sendKV("T", currentTemp, 1);
    uart.sendKV("H", currentHumid, 1);
    uart.sendKV("FAN1", fanA.isOn() ? 1 : 0);
    uart.sendKV("FAN2", fanB.isOn() ? 1 : 0);
    uart.sendKV("DOOR", servoDoor.isOpen() ? 1 : 0);
    uart.sendKV("LOCK", doorLock.isLocked() ? 1 : 0);

    // Door presence alert for web/UI (1 when someone stands <=15cm for >30s)
    uart.sendKV("PRES", g_presenceAlerted ? 1 : 0);

    // Extra actuator state
    uart.sendKV("CUR", curtain.motion());
    uart.sendKV("RBW", statusLED.rainbowTarget() ? 1 : 0);
    uart.sendKV("LEDM", (int)statusLED.segmentMask());

    // LED color id per segment (0=WHITE,1=RED,2=GREEN,3=BLUE)
    uart.sendKV("LC1", (int)statusLED.segmentColorId(0));
    uart.sendKV("LC2", (int)statusLED.segmentColorId(1));
    uart.sendKV("LC3", (int)statusLED.segmentColorId(2));
  }
}
