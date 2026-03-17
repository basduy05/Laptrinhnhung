#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>

// USB-Serial bridge protocol:
// - ESP32 prints JSON lines (type=state) periodically to Serial
// - PC sends plain command lines (e.g. LED1_ON, FAN, OFF, 1, 2, 3, A, B, *) to Serial

// --- 1) MP3 (Serial1) ---
#define MP3_RX_PIN 4
#define MP3_TX_PIN 5
#define FPSerial Serial1

DFRobotDFPlayerMini myDFPlayer;
int currentVol = 20;

// --- 2) LCD 20x4 (I2C) ---
LiquidCrystal_I2C lcd(0x27, 20, 4);

// --- 3) KEYPAD 4x4 ---
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// --- 4) Sensors placeholders (change pins/logic for your hardware) ---
const int PIN_GAS_ANALOG = 34;   // ADC1 pin recommended
const int PIN_FIRE_DIGITAL = 35; // input-only ok

// --- 4.2) Extra sensors (optional) ---
// HC-SR04 style ultrasonic (TRIG/ECHO)
const int PIN_US_TRIG = -1; // set your pin, e.g. 21
const int PIN_US_ECHO = -1; // set your pin, e.g. 22
// LDR analog input
const int PIN_LDR_ANALOG = -1; // set your pin, e.g. 32 (ADC1)

// --- 4.1) Actuators (CHANGE THESE PINS to match your wiring) ---
// Notes:
// - Avoid pins already used by Keypad/MP3/I2C.
// - If you plan to use SPI RFID (MFRC522), pins 18/19/23 might be used by SPI.
const int PIN_LIGHT_1 = 16; // Floor 1 light relay
const int PIN_LIGHT_2 = 17; // Floor 2 light relay
const int PIN_LIGHT_3 = 18; // Floor 3 light relay
const int PIN_FAN_1   = 19; // Fan 1 relay
const int PIN_FAN_2   = 23; // Fan 2 relay

// Relay logic: many relay boards are ACTIVE LOW. If yours is ACTIVE LOW, set to true.
const bool RELAY_ACTIVE_LOW = true;

bool light1On = false;
bool light2On = false;
bool light3On = false;
bool fan1On = false;
bool fan2On = false;

// Curtain (software model; wire motor driver later)
bool curtainOpen = false;
String curtainState = "STOP"; // OPEN / CLOSE / STOP

// Door + RFID (simple software model; integrate hardware reader later)
bool doorOpen = false;
unsigned long doorLastAtMs = 0;
String doorLastMethod = ""; // RFID / PIN / MANUAL

// Light colors (UI only unless you wire RGB hardware)
String light1Color = "YELLOW";
String light2Color = "YELLOW";
String light3Color = "YELLOW";
String lightsAllColor = "YELLOW";

Preferences prefs;
static const char* PREF_NS = "rfid";
static const char* PREF_KEY_CARDS = "cards";

static const int MAX_RFID_CARDS = 20;
String rfidCards[MAX_RFID_CARDS];
int rfidCardCount = 0;

static const char* DOOR_PREF_NS = "door";
static const char* DOOR_PREF_PIN = "pin";
static const char* DOOR_PREF_EMERGENCY_PIN = "epin";

String doorPinCode = "1234";
String doorEmergencyPin = "";

static void loadDoorPins() {
  prefs.begin(DOOR_PREF_NS, true);
  String p = prefs.getString(DOOR_PREF_PIN, "1234");
  String e = prefs.getString(DOOR_PREF_EMERGENCY_PIN, "");
  prefs.end();
  p.trim();
  e.trim();
  if (p.length() == 0) p = "1234";
  doorPinCode = p;
  doorEmergencyPin = e;
}

static void saveDoorPin(const String& p) {
  prefs.begin(DOOR_PREF_NS, false);
  prefs.putString(DOOR_PREF_PIN, p);
  prefs.end();
}

static void saveDoorEmergencyPin(const String& p) {
  prefs.begin(DOOR_PREF_NS, false);
  prefs.putString(DOOR_PREF_EMERGENCY_PIN, p);
  prefs.end();
}

static String normalizeUid(String uid) {
  uid.trim();
  uid.toUpperCase();
  uid.replace(" ", "");
  return uid;
}

static void loadRfidCards() {
  rfidCardCount = 0;
  prefs.begin(PREF_NS, true);
  String raw = prefs.getString(PREF_KEY_CARDS, "");
  prefs.end();

  raw.trim();
  if (raw.length() == 0) return;

  int start = 0;
  while (start < raw.length() && rfidCardCount < MAX_RFID_CARDS) {
    int comma = raw.indexOf(',', start);
    if (comma < 0) comma = raw.length();
    String part = raw.substring(start, comma);
    part = normalizeUid(part);
    if (part.length() > 0) {
      rfidCards[rfidCardCount++] = part;
    }
    start = comma + 1;
  }
}

static void saveRfidCards() {
  String out = "";
  for (int i = 0; i < rfidCardCount; i++) {
    if (i) out += ",";
    out += rfidCards[i];
  }
  prefs.begin(PREF_NS, false);
  prefs.putString(PREF_KEY_CARDS, out);
  prefs.end();
}

static bool rfidHasCard(const String& uid) {
  for (int i = 0; i < rfidCardCount; i++) {
    if (rfidCards[i] == uid) return true;
  }
  return false;
}

static bool rfidAddCard(const String& uid) {
  if (uid.length() == 0) return false;
  if (rfidHasCard(uid)) return true;
  if (rfidCardCount >= MAX_RFID_CARDS) return false;
  rfidCards[rfidCardCount++] = uid;
  saveRfidCards();
  return true;
}

static bool rfidDeleteCard(const String& uid) {
  for (int i = 0; i < rfidCardCount; i++) {
    if (rfidCards[i] == uid) {
      for (int j = i; j < rfidCardCount - 1; j++) {
        rfidCards[j] = rfidCards[j + 1];
      }
      rfidCardCount--;
      saveRfidCards();
      return true;
    }
  }
  return false;
}

static void printRfidCardsJson() {
  Serial.print("{\"type\":\"rfid_cards\",\"cards\":[");
  for (int i = 0; i < rfidCardCount; i++) {
    if (i) Serial.print(",");
    Serial.print("\"");
    Serial.print(rfidCards[i]);
    Serial.print("\"");
  }
  Serial.print("],\"updated_at\":\"ms:");
  Serial.print(millis());
  Serial.println("\"}");
}

static void printRfidScanJson(const String& uid, bool authorized, const String& method) {
  Serial.print("{\"type\":\"rfid_scan\",\"uid\":\"");
  Serial.print(uid);
  Serial.print("\",\"authorized\":");
  Serial.print(authorized ? "true" : "false");
  Serial.print(",\"method\":\"");
  Serial.print(method);
  Serial.print("\",\"at_ms\":");
  Serial.print(millis());
  Serial.println("}");
}

float g_tempC = NAN;             // add your DHT/DS18B20 later
float g_humPct = NAN;
int g_gasValue = -1;
String g_fireStatus = "Unknown"; // Safe / Alert / Unknown

int g_ldrValue = -1;
float g_ultrasonicCm = NAN;

unsigned long lastSensorMs = 0;
unsigned long lastStatePrintMs = 0;
const unsigned long SENSOR_INTERVAL_MS = 1000;
const unsigned long STATE_PRINT_INTERVAL_MS = 1000;

String serialLine;

static void printStateJson() {
  // JSON on one line (PC bridge reads these)
  Serial.print("{\"type\":\"state\"");

  if (isnan(g_tempC)) Serial.print(",\"temperature_c\":null");
  else { Serial.print(",\"temperature_c\":"); Serial.print(g_tempC, 2); }

  if (isnan(g_humPct)) Serial.print(",\"humidity_percent\":null");
  else { Serial.print(",\"humidity_percent\":"); Serial.print(g_humPct, 2); }

  if (g_gasValue < 0) Serial.print(",\"gas_value\":null");
  else { Serial.print(",\"gas_value\":"); Serial.print(g_gasValue); }

  if (g_ldrValue < 0) Serial.print(",\"ldr_value\":null");
  else { Serial.print(",\"ldr_value\":"); Serial.print(g_ldrValue); }

  if (isnan(g_ultrasonicCm)) Serial.print(",\"ultrasonic_cm\":null");
  else { Serial.print(",\"ultrasonic_cm\":"); Serial.print(g_ultrasonicCm, 1); }

  Serial.print(",\"fire_status\":\"");
  Serial.print(g_fireStatus);
  Serial.print("\"");

  Serial.print(",\"updated_at\":\"ms:");
  Serial.print(millis());

  Serial.print("\",\"devices\":{");
  Serial.print("\"light1\":"); Serial.print(light1On ? "true" : "false");
  Serial.print(",\"light2\":"); Serial.print(light2On ? "true" : "false");
  Serial.print(",\"light3\":"); Serial.print(light3On ? "true" : "false");
  Serial.print(",\"fan1\":"); Serial.print(fan1On ? "true" : "false");
  Serial.print(",\"fan2\":"); Serial.print(fan2On ? "true" : "false");
  Serial.print(",\"curtain_open\":"); Serial.print(curtainOpen ? "true" : "false");
  Serial.print(",\"curtain_state\":\""); Serial.print(curtainState); Serial.print("\"");
  Serial.print(",\"door_open\":"); Serial.print(doorOpen ? "true" : "false");
  Serial.print(",\"door_last_method\":\""); Serial.print(doorLastMethod); Serial.print("\"");
  Serial.print(",\"door_emergency_pin_set\":"); Serial.print(doorEmergencyPin.length() > 0 ? "true" : "false");
  Serial.print(",\"light1_color\":\""); Serial.print(light1Color); Serial.print("\"");
  Serial.print(",\"light2_color\":\""); Serial.print(light2Color); Serial.print("\"");
  Serial.print(",\"light3_color\":\""); Serial.print(light3Color); Serial.print("\"");
  Serial.print(",\"lights_all_color\":\""); Serial.print(lightsAllColor); Serial.print("\"");
  Serial.print("}}");
  Serial.println();
}

static void updateSensors() {
  if (millis() - lastSensorMs < SENSOR_INTERVAL_MS) return;
  lastSensorMs = millis();

  g_gasValue = analogRead(PIN_GAS_ANALOG);

  if (PIN_LDR_ANALOG >= 0) {
    g_ldrValue = analogRead(PIN_LDR_ANALOG);
  } else {
    g_ldrValue = -1;
  }

  if (PIN_US_TRIG >= 0 && PIN_US_ECHO >= 0) {
    digitalWrite(PIN_US_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_US_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_US_TRIG, LOW);
    unsigned long duration = pulseIn((uint8_t)PIN_US_ECHO, HIGH, 30000UL);
    if (duration == 0) {
      g_ultrasonicCm = NAN;
    } else {
      g_ultrasonicCm = (float)duration * 0.0343f / 2.0f;
    }
  } else {
    g_ultrasonicCm = NAN;
  }

  // NOTE: some flame sensors output LOW when flame detected.
  int fireRaw = digitalRead(PIN_FIRE_DIGITAL);
  g_fireStatus = (fireRaw == HIGH) ? "Safe" : "Alert";
}

static void handleAction(const String& cmd) {
  auto relayWrite = [](int pin, bool on) {
    if (pin < 0) return;
    if (RELAY_ACTIVE_LOW) {
      digitalWrite(pin, on ? LOW : HIGH);
    } else {
      digitalWrite(pin, on ? HIGH : LOW);
    }
  };

  // PC bridge can request an immediate state push
  if (cmd == "STATE") {
    printStateJson();
    return;
  }

  // --- Door commands ---
  if (cmd == "DOOR_OPEN") {
    doorOpen = true;
    doorLastAtMs = millis();
    doorLastMethod = "MANUAL";
    lcd.setCursor(0, 3);
    lcd.print("Door: OPEN          ");
    return;
  }
  if (cmd == "DOOR_CLOSE") {
    doorOpen = false;
    doorLastAtMs = millis();
    doorLastMethod = "MANUAL";
    lcd.setCursor(0, 3);
    lcd.print("Door: CLOSE         ");
    return;
  }

  if (cmd.startsWith("DOOR_PIN_SET:")) {
    String pin = cmd.substring(String("DOOR_PIN_SET:").length());
    pin.trim();
    if (pin.length() > 0) {
      doorPinCode = pin;
      saveDoorPin(pin);
      lcd.setCursor(0, 3);
      lcd.print("Door PIN: SAVED     ");
    }
    return;
  }

  if (cmd.startsWith("DOOR_PIN_EMERGENCY_SET:")) {
    String pin = cmd.substring(String("DOOR_PIN_EMERGENCY_SET:").length());
    pin.trim();
    if (pin.length() > 0) {
      doorEmergencyPin = pin;
      saveDoorEmergencyPin(pin);
      lcd.setCursor(0, 3);
      lcd.print("E-PIN: SAVED        ");
    }
    return;
  }

  // --- Curtain commands ---
  if (cmd == "CURTAIN_OPEN") {
    curtainOpen = true;
    curtainState = "OPEN";
    lcd.setCursor(0, 3);
    lcd.print("Curtain: OPEN       ");
    return;
  }
  if (cmd == "CURTAIN_CLOSE") {
    curtainOpen = false;
    curtainState = "CLOSE";
    lcd.setCursor(0, 3);
    lcd.print("Curtain: CLOSE      ");
    return;
  }
  if (cmd == "CURTAIN_STOP") {
    curtainState = "STOP";
    lcd.setCursor(0, 3);
    lcd.print("Curtain: STOP       ");
    return;
  }
  if (cmd.startsWith("DOOR_PIN:")) {
    String pin = cmd.substring(String("DOOR_PIN:").length());
    pin.trim();
    bool okNormal = (pin == doorPinCode);
    bool okEmergency = (doorEmergencyPin.length() > 0 && pin == doorEmergencyPin);
    if (okNormal || okEmergency) {
      doorOpen = true;
      doorLastMethod = okEmergency ? "EMERGENCY_PIN" : "PIN";
      doorLastAtMs = millis();
      lcd.setCursor(0, 3);
      lcd.print(okEmergency ? "Door: OPEN (E-PIN) " : "Door: OPEN (PIN)    ");
      printRfidScanJson("PIN", true, okEmergency ? "E-PIN" : "PIN");
    } else {
      lcd.setCursor(0, 3);
      lcd.print("Door PIN: FAIL      ");
      printRfidScanJson("PIN", false, "PIN");
    }
    return;
  }

  // --- RFID management (via serial commands; UI uses bridge endpoints) ---
  if (cmd == "RFID_LIST") {
    printRfidCardsJson();
    return;
  }
  if (cmd.startsWith("RFID_ADD:")) {
    String uid = normalizeUid(cmd.substring(String("RFID_ADD:").length()));
    rfidAddCard(uid);
    printRfidCardsJson();
    return;
  }
  if (cmd.startsWith("RFID_DEL:")) {
    String uid = normalizeUid(cmd.substring(String("RFID_DEL:").length()));
    rfidDeleteCard(uid);
    printRfidCardsJson();
    return;
  }
  // Simulation hook: PC can send RFID_SCAN:UID to test without hardware reader
  if (cmd.startsWith("RFID_SCAN:")) {
    String uid = normalizeUid(cmd.substring(String("RFID_SCAN:").length()));
    bool authorized = rfidHasCard(uid);
    if (authorized) {
      doorOpen = true;
      doorLastMethod = "RFID";
      doorLastAtMs = millis();
      lcd.setCursor(0, 3);
      lcd.print("RFID OK -> OPEN     ");
    } else {
      lcd.setCursor(0, 3);
      lcd.print("RFID FAIL           ");
    }
    printRfidScanJson(uid, authorized, "RFID");
    return;
  }

  // --- Light color commands (UI-only state; wire RGB later) ---
  if (cmd.startsWith("LIGHTS_ALL_COLOR_")) {
    String c = cmd.substring(String("LIGHTS_ALL_COLOR_").length());
    c.trim();
    c.toUpperCase();
    lightsAllColor = c;

    // UX expectation: choosing an "all floors" color implies the lights are ON.
    // This also keeps dashboard state consistent even if only COLOR_* is sent (e.g., AI/voice).
    light1On = true; light2On = true; light3On = true;
    relayWrite(PIN_LIGHT_1, true);
    relayWrite(PIN_LIGHT_2, true);
    relayWrite(PIN_LIGHT_3, true);

    if (c == "RAINBOW") {
      light1Color = "YELLOW";
      light2Color = "BLUE";
      light3Color = "PURPLE";
    } else {
      light1Color = c;
      light2Color = c;
      light3Color = c;
    }
    lcd.setCursor(0, 3);
    lcd.print("Lights: COLOR       ");
    return;
  }
  if (cmd == "LIGHTS_ALL_ON") {
    light1On = true; light2On = true; light3On = true;
    relayWrite(PIN_LIGHT_1, true);
    relayWrite(PIN_LIGHT_2, true);
    relayWrite(PIN_LIGHT_3, true);
    lcd.setCursor(0, 3);
    lcd.print("Lights: ALL ON      ");
    return;
  }
  if (cmd == "LIGHTS_ALL_OFF") {
    light1On = false; light2On = false; light3On = false;
    relayWrite(PIN_LIGHT_1, false);
    relayWrite(PIN_LIGHT_2, false);
    relayWrite(PIN_LIGHT_3, false);
    lcd.setCursor(0, 3);
    lcd.print("Lights: ALL OFF     ");
    return;
  }
  if (cmd.startsWith("LIGHT1_COLOR_")) {
    String c = cmd.substring(String("LIGHT1_COLOR_").length());
    c.trim(); c.toUpperCase();
    light1Color = c;
    lcd.setCursor(0, 3);
    lcd.print("Light1: COLOR       ");
    return;
  }
  if (cmd.startsWith("LIGHT2_COLOR_")) {
    String c = cmd.substring(String("LIGHT2_COLOR_").length());
    c.trim(); c.toUpperCase();
    light2Color = c;
    lcd.setCursor(0, 3);
    lcd.print("Light2: COLOR       ");
    return;
  }
  if (cmd.startsWith("LIGHT3_COLOR_")) {
    String c = cmd.substring(String("LIGHT3_COLOR_").length());
    c.trim(); c.toUpperCase();
    light3Color = c;
    lcd.setCursor(0, 3);
    lcd.print("Light3: COLOR       ");
    return;
  }

  // Accept both your old keypad actions and your dashboard HTTP commands
  if (cmd == "LED1_ON") {
    light1On = true;
    relayWrite(PIN_LIGHT_1, true);
    lcd.setCursor(0, 3);
    lcd.print("Light: ON           ");
    return;
  }
  if (cmd == "LED1_OFF") {
    light1On = false;
    relayWrite(PIN_LIGHT_1, false);
    lcd.setCursor(0, 3);
    lcd.print("Light: OFF          ");
    return;
  }
  if (cmd == "FAN") {
    fan1On = !fan1On;
    relayWrite(PIN_FAN_1, fan1On);
    lcd.setCursor(0, 3);
    lcd.print("Fan: TOGGLE         ");
    return;
  }
  if (cmd == "FAN_OFF") {
    fan1On = false;
    relayWrite(PIN_FAN_1, false);
    lcd.setCursor(0, 3);
    lcd.print("Fan1: OFF           ");
    return;
  }
  if (cmd == "OFF") {
    myDFPlayer.stop();
    light1On = false; light2On = false; light3On = false;
    fan1On = false; fan2On = false;
    relayWrite(PIN_LIGHT_1, false);
    relayWrite(PIN_LIGHT_2, false);
    relayWrite(PIN_LIGHT_3, false);
    relayWrite(PIN_FAN_1, false);
    relayWrite(PIN_FAN_2, false);
    lcd.setCursor(0, 3);
    lcd.print("EMERGENCY STOP      ");
    return;
  }

  // Extra commands for more devices
  if (cmd == "LIGHT1_ON") { light1On = true; relayWrite(PIN_LIGHT_1, true); lcd.setCursor(0, 3); lcd.print("Light1: ON          "); return; }
  if (cmd == "LIGHT1_OFF") { light1On = false; relayWrite(PIN_LIGHT_1, false); lcd.setCursor(0, 3); lcd.print("Light1: OFF         "); return; }
  if (cmd == "LIGHT2_ON") { light2On = true; relayWrite(PIN_LIGHT_2, true); lcd.setCursor(0, 3); lcd.print("Light2: ON          "); return; }
  if (cmd == "LIGHT2_OFF") { light2On = false; relayWrite(PIN_LIGHT_2, false); lcd.setCursor(0, 3); lcd.print("Light2: OFF         "); return; }
  if (cmd == "LIGHT3_ON") { light3On = true; relayWrite(PIN_LIGHT_3, true); lcd.setCursor(0, 3); lcd.print("Light3: ON          "); return; }
  if (cmd == "LIGHT3_OFF") { light3On = false; relayWrite(PIN_LIGHT_3, false); lcd.setCursor(0, 3); lcd.print("Light3: OFF         "); return; }

  if (cmd == "FAN1_ON") { fan1On = true; relayWrite(PIN_FAN_1, true); lcd.setCursor(0, 3); lcd.print("Fan1: ON            "); return; }
  if (cmd == "FAN1_OFF") { fan1On = false; relayWrite(PIN_FAN_1, false); lcd.setCursor(0, 3); lcd.print("Fan1: OFF           "); return; }
  if (cmd == "FAN2_ON") { fan2On = true; relayWrite(PIN_FAN_2, true); lcd.setCursor(0, 3); lcd.print("Fan2: ON            "); return; }
  if (cmd == "FAN2_OFF") { fan2On = false; relayWrite(PIN_FAN_2, false); lcd.setCursor(0, 3); lcd.print("Fan2: OFF           "); return; }
  if (cmd == "FAN2_TOGGLE") { fan2On = !fan2On; relayWrite(PIN_FAN_2, fan2On); lcd.setCursor(0, 3); lcd.print("Fan2: TOGGLE        "); return; }

  // Keypad-like MP3 control
  if (cmd == "1" || cmd == "2" || cmd == "3") {
    int track = cmd.toInt();
    lcd.setCursor(0, 3);
    lcd.print("Playing Track 000");
    lcd.print(track);
    lcd.print("  ");
    myDFPlayer.play(track);
    return;
  }
  if (cmd == "A") {
    if (currentVol < 30) currentVol++;
    myDFPlayer.volume(currentVol);
    lcd.setCursor(0, 3);
    lcd.print(("Volume Up: " + String(currentVol) + "       ").c_str());
    return;
  }
  if (cmd == "B") {
    if (currentVol > 0) currentVol--;
    myDFPlayer.volume(currentVol);
    lcd.setCursor(0, 3);
    lcd.print(("Volume Down: " + String(currentVol) + "     ").c_str());
    return;
  }
  if (cmd == "*") {
    myDFPlayer.stop();
    lcd.setCursor(0, 3);
    lcd.print("Music Stopped       ");
    return;
  }
}

void setup() {
  Serial.begin(115200);

  loadRfidCards();
  loadDoorPins();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("   MEGAHOME SYSTEM  ");
  lcd.setCursor(0, 1);
  lcd.print("USB Bridge Mode     ");

  pinMode(PIN_FIRE_DIGITAL, INPUT);

  pinMode(PIN_LIGHT_1, OUTPUT);
  pinMode(PIN_LIGHT_2, OUTPUT);
  pinMode(PIN_LIGHT_3, OUTPUT);
  pinMode(PIN_FAN_1, OUTPUT);
  pinMode(PIN_FAN_2, OUTPUT);

  // Set all relays OFF at boot
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(PIN_LIGHT_1, HIGH);
    digitalWrite(PIN_LIGHT_2, HIGH);
    digitalWrite(PIN_LIGHT_3, HIGH);
    digitalWrite(PIN_FAN_1, HIGH);
    digitalWrite(PIN_FAN_2, HIGH);
  } else {
    digitalWrite(PIN_LIGHT_1, LOW);
    digitalWrite(PIN_LIGHT_2, LOW);
    digitalWrite(PIN_LIGHT_3, LOW);
    digitalWrite(PIN_FAN_1, LOW);
    digitalWrite(PIN_FAN_2, LOW);
  }

  // MP3 init
  FPSerial.begin(9600, SERIAL_8N1, MP3_RX_PIN, MP3_TX_PIN);
  delay(800);

  if (!myDFPlayer.begin(FPSerial)) {
    lcd.setCursor(0, 2);
    lcd.print("MP3 Error           ");
  } else {
    lcd.setCursor(0, 2);
    lcd.print("MP3 Ready           ");
    myDFPlayer.volume(currentVol);
    myDFPlayer.play(1);
  }

  lcd.setCursor(0, 3);
  lcd.print("Waiting PC...       ");
}

void loop() {
  // 1) Update sensors + print state
  updateSensors();
  if (millis() - lastStatePrintMs >= STATE_PRINT_INTERVAL_MS) {
    lastStatePrintMs = millis();
    printStateJson();
  }

  // 2) Keypad
  char k = customKeypad.getKey();
  if (k) {
    lcd.setCursor(0, 2);
    lcd.print("Key: ");
    lcd.print(k);
    lcd.print("              ");

    handleAction(String(k));
  }

  // 3) USB serial commands
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      String cmd = serialLine;
      serialLine = "";
      cmd.trim();
      if (cmd.length() > 0) {
        handleAction(cmd);
      }
    } else {
      if (serialLine.length() < 128) serialLine += c;
    }
  }
}
