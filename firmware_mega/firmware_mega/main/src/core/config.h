#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

/*************************************************
 * SYSTEM CONFIGURATION
 *************************************************/
#define SYSTEM_NAME        "SMART_HOME_MEGA"
#define FIRMWARE_VERSION  "1.0.0"
#define SERIAL_BAUDRATE   115200

/*************************************************
 * PIN MAP - ACTUATORS
 *************************************************/

// Status LED (NeoPixel)
#define PIN_STATUS_LED       7
#define STATUS_LED_COUNT     18
#define STATUS_LED_MAX_BRIGHTNESS  50  // Balance: bright enough but prevents brownout

// Fan relays (active LOW)
#define PIN_RELAY_FAN        34
#define PIN_RELAY_SOCKET     35

// Relay polarity (set to 0 if your relay is active HIGH)
#define FAN_RELAY_ACTIVE_LOW   0

// Servo - Door
#define PIN_SERVO_DOOR       6

// Door servo calibration angles (degrees 0..180)
// Adjust these values to match your physical door.
#define SERVO_DOOR_OPEN_ANGLE   130
#define SERVO_DOOR_CLOSE_ANGLE  0

// Door automation
// 1 = Mega auto-opens door when it receives PIN_OK / RFID_HELLO from ESP32.
#define DOOR_AUTO_OPEN_ON_AUTH   1
// 1 = Mega auto-closes the door after it has been opened for a while.
#define DOOR_AUTO_CLOSE_ENABLE   1
// Auto-close delay (ms)
#define DOOR_AUTO_CLOSE_MS       8000UL

// Close after emergency/hostage voice finishes
// 1 = when gas/fire emergency clears, Mega will close+lock after a delay.
#define DOOR_CLOSE_AFTER_EMERGENCY  1
// Delay (ms) to wait for ESP32 voice ("he thong an toan") before closing.
#define DOOR_CLOSE_AFTER_EMERGENCY_DELAY_MS  5000UL

// 1 = when ESP32 signals HOSTAGE_CLEAR, Mega closes+locks after a delay.
#define DOOR_CLOSE_ON_HOSTAGE_CLEAR 1
// Delay (ms) to wait for ESP32 voice before closing.
#define DOOR_CLOSE_ON_HOSTAGE_CLEAR_DELAY_MS  5000UL

// Servo power relay (cuts +V to servo). Use this to avoid keeping the servo powered continuously.
// Wire relay COM <- +V servo supply, relay NO -> servo V+ (red). Servo GND must be common with Mega.
#define PIN_RELAY_SERVO_PWR  32
// 1 = relay input active LOW, 0 = active HIGH
#define SERVO_RELAY_ACTIVE_LOW  0

// Debug helpers (set to 1 temporarily for troubleshooting)
// 1 = run a quick open/close test on boot
#define SERVO_DIAG_BOOT_TEST          0
// 1 = ignore ultrasonic door safety gate (g_doorFar) to allow servo movement
#define SERVO_DIAG_IGNORE_DOOR_SAFETY 0
// 1 = blink servo power relay on boot (verifies wiring/polarity on PIN_RELAY_SERVO_PWR)
#define SERVO_DIAG_RELAY_TEST         0

// 1 = allow Serial Monitor to trigger door actions (for relay/servo emergency testing)
// Commands: 'e' = unlock+open (emergency), 'o' = open, 'c' = close+lock
#define SERVO_DIAG_SERIAL_DOOR_TEST   0

// Stepper - Curtain
#define PIN_STEPPER_IN1      40
#define PIN_STEPPER_IN2      44
#define PIN_STEPPER_IN3      42
#define PIN_STEPPER_IN4      46

// Buzzer
#define PIN_BUZZER           10
// 1 = active LOW buzzer module, 0 = active HIGH
#define BUZZER_ACTIVE_LOW    1
// 1 = use tone()/noTone() (works best for passive buzzers, active HIGH)
#define BUZZER_USE_TONE      1
#define BUZZER_TONE_HZ       2000

// Door Lock (Relay / Solenoid)
#define PIN_DOOR_LOCK        25

/*************************************************
 * PIN MAP - SENSORS
 *************************************************/

// Gas Sensor (MQ-x)
#define PIN_GAS_SENSOR       A0

// Fire Sensor (Flame)
#define PIN_FIRE_SENSOR      9

// IR Sensor
#define PIN_IR_SENSOR        37
// Some IR modules assert HIGH on detect, some LOW
#define IR_DETECTED_LEVEL    HIGH
// Enable internal pullup if your IR OUT is floating
#define IR_USE_PULLUP        1

// Ultrasonic
#define PIN_ULTRASONIC_TRIG  12
#define PIN_ULTRASONIC_ECHO  13
// pulseIn timeout in microseconds (30000us ~ 5m)
#define ULTRASONIC_TIMEOUT_US 30000UL

// DHT
#define PIN_DHT_SENSOR       8
#define DHT_TYPE             DHT11

// LDR
#define PIN_LDR_SENSOR       A1
// 1 = higher ADC means darker, 0 = lower ADC means darker
#define LDR_DARK_WHEN_HIGH   1

// Door Magnetic Switch
#define PIN_DOOR_SWITCH      36
// Door switch logic level when the door is OPEN.
// With INPUT_PULLUP wiring, many reed switches read LOW when closed-to-GND.
// Set to LOW or HIGH based on your wiring.
// If your reed switch closes to GND when the door is OPEN (common with INPUT_PULLUP), OPEN=LOW.
// If your wiring is opposite, change to HIGH.
#define DOOR_SWITCH_OPEN_LEVEL LOW

// Power Monitor (Analog)
#define PIN_POWER_MONITOR    A2

/*************************************************
 * PIN MAP - INPUT
 *************************************************/

// Keypad 4x4
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4

// Declare as extern to avoid multiple definition
extern const uint8_t KEYPAD_ROW_PINS[KEYPAD_ROWS];
extern const uint8_t KEYPAD_COL_PINS[KEYPAD_COLS];

// Emergency Button
#define PIN_EMERGENCY_BUTTON 43

/*************************************************
 * THRESHOLD & TIMING
 *************************************************/

// Gas
#define GAS_DANGER_THRESHOLD     800
// Startup calibration: threshold = baseline + margin
#define GAS_CALIBRATION_SAMPLES  50
#define GAS_CALIBRATION_DELAY_MS 10
#define GAS_CALIBRATION_MARGIN   200

// Temperature (DHT)
#define TEMP_AUTO_FAN_ON         30   // °C - Auto fan in normal mode
#define TEMP_AUTO_FAN_OFF        28   // °C - Hysteresis

// Fire
#define FIRE_DETECTED_LEVEL      LOW

// Ultrasonic
#define INTRUSION_DISTANCE_CM    5

// LDR
#define LDR_THRESHOLD            600

// Anti brute-force
#define MAX_PASSWORD_RETRY       5
#define PASSWORD_LOCK_TIME_MS    30000UL

// Scheduler tick
#define SCHEDULER_TICK_MS        10

// Emergency debounce / anti-chatter (ms)
#define EMERGENCY_ASSERT_MS      500UL
#define EMERGENCY_CLEAR_MS       2000UL

// Sequencing to reduce current spikes (ms)
#define EMERGENCY_UNLOCK_TO_SERVO_MS  250UL

// Servo power saving
#define SERVO_DETACH_AFTER_MS        2000UL  // Detach after 2s idle

// Note: when the servo detaches (power saving), the door may drift if there's load/spring.
// Increase SERVO_DETACH_AFTER_MS or disable power saving in main.ino if you need it to hold angle.

/*************************************************
 * UART PROTOCOL
 *************************************************/
#define UART_START_CHAR   '<'
#define UART_END_CHAR     '>'
#define UART_SEPARATOR    ':'

/*************************************************
 * SYSTEM MODE & PRIORITY
 *************************************************/
enum SystemMode {
    MODE_AUTO = 0,
    MODE_MANUAL,
    MODE_EMERGENCY
};

enum PriorityLevel {
    PRIORITY_AUTO = 0,
    PRIORITY_MANUAL,
    PRIORITY_EMERGENCY
};

#endif // CONFIG_H
