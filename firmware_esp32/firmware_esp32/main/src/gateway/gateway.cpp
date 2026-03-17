#include "gateway.h"

#include <ArduinoJson.h>
#include <math.h>
#include <Preferences.h>
#include <time.h>

#include "../devices/device_shadow.h"
#include "../devices/rfid_mgr.h"
#include "../devices/audio_mgr.h"
#include "../devices/audio_tracks.h"
#include "../comm/uart_link.h"

extern RFIDMgr rfid;
extern UARTLink uart;

namespace {

static const int MAX_RFID_CARDS = 20;
static const int MAX_RFID_HISTORY = 20;
static const int MAX_PINS = 12;
static const int MAX_EMERGENCY_PINS = 4;

struct RfidEntry {
  char uid[24];
  char name[32];
  bool inside;
};

struct RfidScan {
  char uid[24];
  char name[32];
  bool authorized;
  char direction[8];
  long epoch;
  unsigned long tsMs;
};

struct PinEntry {
  char pin[16];
};

struct EmergencyPinEntry {
  char pin[16];
};

static RfidEntry g_cards[MAX_RFID_CARDS];
static int g_cardCount = 0;

static PinEntry g_pins[MAX_PINS];
static int g_pinCount = 0;

// Master/admin PIN used for keypad door access + admin sequence (*PIN#)
static char g_masterPin[9] = "123456"; // 4..8 digits + NUL

static EmergencyPinEntry g_epins[MAX_EMERGENCY_PINS];
static int g_epinCount = 0;

static RfidScan g_history[MAX_RFID_HISTORY];
static int g_histCount = 0;

static RfidScan g_lastScan;
static bool g_hasLast = false;

static uint32_t g_seq = 1;
static unsigned long g_lastStateMs = 0;
static bool g_capsSent = false;

static String g_lastDoorMethod = "";

// Hostage/emergency-PIN escalation state
static bool g_hostageActive = false;
static unsigned long g_hostageStartMs = 0;
static unsigned long g_hostageNextBeepMs = 0;
static bool g_hostageEscalated = false;

static Preferences g_prefs;
static bool g_prefsStarted = false;

static bool uidEquals_(const char* a, const char* b) {
  return a && b && strcmp(a, b) == 0;
}

static void prefsEnsure_() {
  if (g_prefsStarted) return;
  g_prefsStarted = true;
  (void)g_prefs.begin("miki", false);
}

static bool uidValid_(const String& uid) {
  if (uid.length() < 4 || uid.length() > 20) return false;
  for (size_t i = 0; i < uid.length(); i++) {
    const char c = uid[i];
    const bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
    if (!ok) return false;
  }
  return true;
}

static String nameSanitize_(String name) {
  name.trim();
  if (name.length() > 31) name = name.substring(0, 31);
  // Basic ASCII filtering to keep JSON + storage simple.
  for (size_t i = 0; i < name.length(); i++) {
    const char c = name[i];
    if (c < 0x20 || c == '"' || c == '\\') name.setCharAt(i, ' ');
  }
  name.trim();
  return name;
}

static bool pinValid_(const String& pin) {
  if (pin.length() < 4 || pin.length() > 12) return false;
  for (size_t i = 0; i < pin.length(); i++) {
    const char c = pin[i];
    if (c < '0' || c > '9') return false;
  }
  return true;
}

static bool pinEquals_(const char* a, const char* b) {
  return a && b && strcmp(a, b) == 0;
}

static bool isPinAuthorized_(const char* pin) {
  if (pinEquals_(g_masterPin, pin)) return true;
  for (int i = 0; i < g_pinCount; i++) {
    if (pinEquals_(g_pins[i].pin, pin)) return true;
  }
  return false;
}

static bool isEmergencyPinAuthorized_(const char* pin) {
  for (int i = 0; i < g_epinCount; i++) {
    if (pinEquals_(g_epins[i].pin, pin)) return true;
  }
  return false;
}

static void hostageStart_() {
  g_hostageActive = true;
  g_hostageStartMs = millis();
  g_hostageNextBeepMs = g_hostageStartMs + 5000UL;
  g_hostageEscalated = false;
  uart.send("CMD:HOSTAGE_ON");
}

static void hostageClear_() {
  g_hostageActive = false;
  g_hostageStartMs = 0;
  g_hostageNextBeepMs = 0;
  g_hostageEscalated = false;
  uart.send("CMD:HOSTAGE_CLEAR");
}

static long nowEpoch_() {
  const time_t now = time(nullptr);
  // If NTP not ready, ESP32 often returns 0 or 1.
  if (now < 1700000000L) return 0;
  return (long)now;
}

static void saveCards_() {
  prefsEnsure_();
  StaticJsonDocument<2048> doc;
  JsonArray arr = doc.createNestedArray("cards");
  for (int i = 0; i < g_cardCount; i++) {
    JsonObject o = arr.createNestedObject();
    o["uid"] = g_cards[i].uid;
    o["name"] = g_cards[i].name;
  }
  String out;
  serializeJson(doc, out);
  (void)g_prefs.putString("cards", out);
}

static void savePins_() {
  prefsEnsure_();
  StaticJsonDocument<384> doc;
  JsonArray arr = doc.createNestedArray("pins");
  for (int i = 0; i < g_pinCount; i++) arr.add(g_pins[i].pin);
  String out;
  serializeJson(doc, out);
  (void)g_prefs.putString("pins", out);

  // Save master pin separately
  (void)g_prefs.putString("mpin", String(g_masterPin));
}

static void saveEmergencyPins_() {
  prefsEnsure_();
  StaticJsonDocument<256> doc;
  JsonArray arr = doc.createNestedArray("pins");
  for (int i = 0; i < g_epinCount; i++) arr.add(g_epins[i].pin);
  String out;
  serializeJson(doc, out);
  (void)g_prefs.putString("epins", out);
}

static void loadCards_() {
  prefsEnsure_();
  const String json = g_prefs.getString("cards", "");
  if (json.length() == 0) return;

  StaticJsonDocument<2048> doc;
  if (deserializeJson(doc, json)) return;
  JsonArray arr = doc["cards"].as<JsonArray>();
  if (arr.isNull()) return;

  for (JsonVariant v : arr) {
    const char* uidC = v["uid"] | "";
    const char* nameC = v["name"] | "";
    String uid(uidC);
    uid.trim();
    uid.toUpperCase();
    if (!uidValid_(uid)) continue;
    if (g_cardCount >= MAX_RFID_CARDS) break;

    memset(&g_cards[g_cardCount], 0, sizeof(RfidEntry));
    strncpy(g_cards[g_cardCount].uid, uid.c_str(), sizeof(g_cards[g_cardCount].uid) - 1);
    String name = nameSanitize_(String(nameC));
    strncpy(g_cards[g_cardCount].name, name.c_str(), sizeof(g_cards[g_cardCount].name) - 1);
    g_cards[g_cardCount].inside = false;
    g_cardCount++;
  }
}

static void loadPins_() {
  prefsEnsure_();

  // Load master pin first (default stays if missing/invalid)
  {
    String mp = g_prefs.getString("mpin", "");
    mp.trim();
    if (pinValid_(mp)) {
      strncpy(g_masterPin, mp.c_str(), sizeof(g_masterPin) - 1);
      g_masterPin[sizeof(g_masterPin) - 1] = '\0';
    }
  }

  const String json = g_prefs.getString("pins", "");
  if (json.length() == 0) return;

  StaticJsonDocument<384> doc;
  if (deserializeJson(doc, json)) return;
  JsonArray arr = doc["pins"].as<JsonArray>();
  if (arr.isNull()) return;

  for (JsonVariant v : arr) {
    String pin = String((const char*)v.as<const char*>());
    pin.trim();
    if (!pinValid_(pin)) continue;
    if (g_pinCount >= MAX_PINS) break;
    memset(&g_pins[g_pinCount], 0, sizeof(PinEntry));
    strncpy(g_pins[g_pinCount].pin, pin.c_str(), sizeof(g_pins[g_pinCount].pin) - 1);
    g_pinCount++;
  }
}

static void loadEmergencyPins_() {
  prefsEnsure_();
  g_epinCount = 0;
  const String json = g_prefs.getString("epins", "");
  if (json.length() == 0) return;

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, json)) return;
  JsonArray arr = doc["pins"].as<JsonArray>();
  if (arr.isNull()) return;

  for (JsonVariant v : arr) {
    String pin = String((const char*)v.as<const char*>());
    pin.trim();
    if (!pinValid_(pin)) continue;
    if (g_epinCount >= MAX_EMERGENCY_PINS) break;
    memset(&g_epins[g_epinCount], 0, sizeof(EmergencyPinEntry));
    strncpy(g_epins[g_epinCount].pin, pin.c_str(), sizeof(g_epins[g_epinCount].pin) - 1);
    g_epinCount++;
  }
}

static bool isAuthorized_(const char* uid) {
  for (int i = 0; i < g_cardCount; i++) {
    if (uidEquals_(g_cards[i].uid, uid)) return true;
  }
  return false;
}

static const char* findNameByUid_(const char* uid) {
  for (int i = 0; i < g_cardCount; i++) {
    if (uidEquals_(g_cards[i].uid, uid)) return g_cards[i].name;
  }
  return "";
}

static bool getInsideByUid_(const char* uid, bool& insideOut) {
  for (int i = 0; i < g_cardCount; i++) {
    if (uidEquals_(g_cards[i].uid, uid)) {
      insideOut = g_cards[i].inside;
      return true;
    }
  }
  return false;
}

static void setInsideByUid_(const char* uid, bool inside) {
  for (int i = 0; i < g_cardCount; i++) {
    if (uidEquals_(g_cards[i].uid, uid)) {
      g_cards[i].inside = inside;
      return;
    }
  }
}

static void pushHistory_(const char* uid, bool authorized) {
  RfidScan s;
  memset(&s, 0, sizeof(s));
  strncpy(s.uid, uid, sizeof(s.uid) - 1);
  strncpy(s.name, findNameByUid_(uid), sizeof(s.name) - 1);
  s.authorized = authorized;
  s.epoch = nowEpoch_();
  s.tsMs = millis();

  if (authorized) {
    bool inside = false;
    (void)getInsideByUid_(uid, inside);
    // Toggle: outside->IN (open), inside->OUT (close)
    const bool nextInside = !inside;
    strncpy(s.direction, nextInside ? "IN" : "OUT", sizeof(s.direction) - 1);
    setInsideByUid_(uid, nextInside);
  } else {
    s.direction[0] = '\0';
  }

  g_lastScan = s;
  g_hasLast = true;

  if (g_histCount < MAX_RFID_HISTORY) {
    g_history[g_histCount++] = s;
    return;
  }

  // shift oldest out
  for (int i = 1; i < MAX_RFID_HISTORY; i++) {
    g_history[i - 1] = g_history[i];
  }
  g_history[MAX_RFID_HISTORY - 1] = s;
}

static void pushHistoryDirect_(const char* uid, bool authorized, const char* name, const char* direction, long epoch) {
  RfidScan s;
  memset(&s, 0, sizeof(s));
  strncpy(s.uid, uid ? uid : "", sizeof(s.uid) - 1);

  const char* nm = (name && name[0] != '\0') ? name : findNameByUid_(uid);
  strncpy(s.name, nm ? nm : "", sizeof(s.name) - 1);

  s.authorized = authorized;
  s.epoch = (epoch != 0) ? epoch : nowEpoch_();
  s.tsMs = millis();

  if (authorized && direction && direction[0] != '\0') {
    strncpy(s.direction, direction, sizeof(s.direction) - 1);
  } else {
    s.direction[0] = '\0';
  }

  g_lastScan = s;
  g_hasLast = true;

  if (g_histCount < MAX_RFID_HISTORY) {
    g_history[g_histCount++] = s;
    return;
  }

  for (int i = 1; i < MAX_RFID_HISTORY; i++) {
    g_history[i - 1] = g_history[i];
  }
  g_history[MAX_RFID_HISTORY - 1] = s;
}

static void serialPrintJson_(const JsonDocument& doc) {
  serializeJson(doc, Serial);
  Serial.print('\n');
}

static void emitAck_(uint32_t id, const String& cmd, bool ok, const char* err) {
  StaticJsonDocument<256> doc;
  doc["type"] = "ack";
  doc["v"] = 1;
  doc["id"] = id;
  doc["cmd"] = cmd;
  doc["ok"] = ok;
  doc["ts_ms"] = (unsigned long)millis();
  if (!ok && err) doc["error"] = err;
  serialPrintJson_(doc);
}

static void emitEvent_(const char* name, JsonDocument& data) {
  StaticJsonDocument<384> doc;
  doc["type"] = "event";
  doc["v"] = 1;
  doc["name"] = name;
  doc["ts_ms"] = (unsigned long)millis();
  doc["data"] = data.as<JsonObject>();
  serialPrintJson_(doc);
}

static void emitRfidScanEvent_(const char* uid, bool authorized, const char* method) {
  StaticJsonDocument<256> data;
  data["uid"] = uid;
  data["authorized"] = authorized;
  data["method"] = method;
  emitEvent_("rfid_scan", data);
}

static String isoMs_(unsigned long ms) {
  // Keep compatibility: bridge/web already accept "ms:<millis>".
  return String("ms:") + ms;
}

static bool canPlayDoorVoice_(bool opening) {
  const unsigned long nowMs = millis();
  const unsigned long lastMs = opening ? shadow.lastDoorOpenVoiceMs : shadow.lastDoorCloseVoiceMs;
  // Single shared cooldown to prevent duplicate announcements when the same action
  // is observed via: command path, Mega events, and telemetry edges.
  return (nowMs - lastMs) > 2500UL;
}

static void markDoorVoicePlayed_(bool opening) {
  const unsigned long nowMs = millis();
  if (opening) shadow.lastDoorOpenVoiceMs = nowMs;
  else shadow.lastDoorCloseVoiceMs = nowMs;
}

static void tryPlayDoorOpenVoice_() {
  // During sensor emergencies (gas/fire), don't announce door open;
  // it can mask the emergency alarm audio.
  if (shadow.emergency) return;
  if (!audioReady()) return;
  if (!canPlayDoorVoice_(true)) return;
  audioPlay(audio_tracks::MOCUA);
  markDoorVoicePlayed_(true);
}

static void tryPlayDoorCloseVoice_() {
  if (!audioReady()) return;
  if (!canPlayDoorVoice_(false)) return;
  audioPlay(audio_tracks::DONGCUA);
  markDoorVoicePlayed_(false);
}

static void handleDoorOpen_(const char* method) {
  g_lastDoorMethod = method;
  // Mega handles unlock+servo
  uart.send("CMD:DOOR_OPEN");

  // Always announce on command (cooldown), so audio still works even if DOOR telemetry is wrong.
  shadow.lastDoorOpenCmdMs = millis();
  tryPlayDoorOpenVoice_();
}

static void handleDoorClose_(const char* method) {
  g_lastDoorMethod = method;
  uart.send("CMD:DOOR_CLOSE");

  shadow.lastDoorCloseCmdMs = millis();
  tryPlayDoorCloseVoice_();
}

static const char* colorIdToStr_(int id) {
  switch (id) {
    case 1: return "RED";
    case 2: return "GREEN";
    case 3: return "BLUE";
    case 4: return "YELLOW";
    case 5: return "PURPLE";
    default: return "WHITE";
  }
}

static bool handleCommandInternal_(const String& cmdIn, String& errOut, const char* source) {
  String cmd = cmdIn;
  cmd.trim();
  cmd.toUpperCase();
  if (cmd.length() == 0) {
    errOut = "empty";
    return false;
  }

  // Door
  if (cmd == "DOOR_OPEN") {
    handleDoorOpen_(source);
    return true;
  }
  if (cmd == "DOOR_CLOSE") {
    handleDoorClose_(source);
    return true;
  }

  // Door events emitted by Mega (used for auto-close / pending-close audio).
  // These must NOT send a command back to Mega.
  if (cmd == "DOOR_OPENED_EVT") {
    g_lastDoorMethod = source;
    // De-dup handled via shared voice timestamp.
    tryPlayDoorOpenVoice_();
    return true;
  }
  if (cmd == "DOOR_CLOSED_EVT") {
    g_lastDoorMethod = source;
    tryPlayDoorCloseVoice_();
    return true;
  }
  if (cmd.startsWith("DOOR_PIN:")) {
    String pin = cmd.substring(9);
    pin.trim();
    if (!pinValid_(pin)) { errOut = "bad_pin_format"; return false; }

    // Emergency PIN (hostage): open door but trigger escalation.
    if (isEmergencyPinAuthorized_(pin.c_str())) {
      handleDoorOpen_("PIN_EMG");
      // Acknowledge keypad flow on Mega.
      uart.send("CMD:PIN_OK");
      hostageStart_();
      StaticJsonDocument<128> data;
      data["pin"] = "****";
      emitEvent_("door_pin_emergency", data);
      return true;
    }

    if (!isPinAuthorized_(pin.c_str())) {
      errOut = "pin_denied";
      if (audioReady()) audioPlay(audio_tracks::SAIMATKHAU);
      uart.send("CMD:PIN_FAIL");
      return false;
    }
    handleDoorOpen_("PIN");
    uart.send("CMD:PIN_OK");
    StaticJsonDocument<128> data;
    data["pin"] = "****";
    emitEvent_("door_pin", data);
    return true;
  }

  // Admin sequence: Mega sends CMD:ADMIN_SEQ:<pin> when user enters *PIN#
  if (cmd.startsWith("ADMIN_SEQ:")) {
    String pin = cmd.substring(10);
    pin.trim();
    if (!pinValid_(pin)) { errOut = "bad_pin_format"; uart.send("CMD:ADMIN_FAIL"); return false; }
    if (!pinEquals_(g_masterPin, pin.c_str())) {
      errOut = "admin_denied";
      uart.send("CMD:ADMIN_FAIL");
      return false;
    }

    // Correct admin PIN may be used to clear hostage/emergency-password mode.
    // Only announce "tat khan cap" when we actually clear that state.
    const bool clearedHostage = g_hostageActive;
    if (clearedHostage) {
      hostageClear_();
      if (audioReady()) audioPlay(audio_tracks::TATKHANCAP);
    }

    uart.send("CMD:ADMIN_OK");
    return true;
  }

  // Admin operations from Mega
  if (cmd.startsWith("PIN_ADD:")) {
    String pin = cmd.substring(8);
    pin.trim();
    const bool ok = gatewayPinAdd(pin);
    uart.send(ok ? "CMD:ADMIN_OK" : "CMD:ADMIN_FAIL");
    if (!ok) errOut = "pin_add_fail";
    return ok;
  }
  if (cmd.startsWith("PIN_DEL:")) {
    String pin = cmd.substring(8);
    pin.trim();
    const bool ok = gatewayPinDel(pin);
    uart.send(ok ? "CMD:ADMIN_OK" : "CMD:ADMIN_FAIL");
    if (!ok) errOut = "pin_del_fail";
    return ok;
  }
  if (cmd.startsWith("EPIN_ADD:")) {
    String pin = cmd.substring(9);
    pin.trim();
    const bool ok = gatewayEmergencyPinAdd(pin);
    uart.send(ok ? "CMD:ADMIN_OK" : "CMD:ADMIN_FAIL");
    if (!ok) errOut = "epin_add_fail";
    return ok;
  }
  if (cmd.startsWith("EPIN_DEL:")) {
    String pin = cmd.substring(9);
    pin.trim();
    const bool ok = gatewayEmergencyPinDel(pin);
    uart.send(ok ? "CMD:ADMIN_OK" : "CMD:ADMIN_FAIL");
    if (!ok) errOut = "epin_del_fail";
    return ok;
  }
  if (cmd.startsWith("MASTER_PIN_SET:")) {
    String pin = cmd.substring(15);
    pin.trim();
    if (!pinValid_(pin)) {
      errOut = "bad_pin_format";
      uart.send("CMD:ADMIN_FAIL");
      return false;
    }
    strncpy(g_masterPin, pin.c_str(), sizeof(g_masterPin) - 1);
    g_masterPin[sizeof(g_masterPin) - 1] = '\0';
    savePins_();
    uart.send("CMD:ADMIN_OK");
    return true;
  }

  // Fans
  if (cmd == "FAN1_ON") { uart.send("CMD:FAN1_ON"); if (audioReady()) audioPlay(audio_tracks::BATQUAT); return true; }
  if (cmd == "FAN1_OFF") { uart.send("CMD:FAN1_OFF"); if (audioReady()) audioPlay(audio_tracks::TATQUAT); return true; }
  if (cmd == "FAN2_ON") { uart.send("CMD:FAN2_ON"); if (audioReady()) audioPlay(audio_tracks::BATQUAT); return true; }
  if (cmd == "FAN2_OFF") { uart.send("CMD:FAN2_OFF"); if (audioReady()) audioPlay(audio_tracks::TATQUAT); return true; }

  // Curtain
  if (cmd == "CURTAIN_OPEN") { uart.send("CMD:CURTAIN_OPEN"); if (audioReady()) audioPlay(audio_tracks::REMLEN); return true; }
  if (cmd == "CURTAIN_CLOSE") { uart.send("CMD:CURTAIN_CLOSE"); if (audioReady()) audioPlay(audio_tracks::REMXUONG); return true; }
  if (cmd == "CURTAIN_STOP") { uart.send("CMD:CURTAIN_STOP"); return true; }

  // Hostage clear
  if (cmd == "HOSTAGE_CLEAR") {
    hostageClear_();
    return true;
  }


  // Emergency OFF (best-effort)
  if (cmd == "OFF") {
    uart.send("CMD:FAN1_OFF");
    uart.send("CMD:FAN2_OFF");
    uart.send("CMD:LED_OFF");
    return true;
  }

  // Lights mapping to Mega LED segments
  if (cmd == "LIGHTS_ALL_ON") { uart.send("CMD:LED_ALL"); if (audioReady()) audioPlay(audio_tracks::BATDEN); return true; }
  if (cmd == "LIGHTS_ALL_OFF") { uart.send("CMD:LED_OFF"); if (audioReady()) audioPlay(audio_tracks::TATDEN); return true; }

  // Convenience aliases (no COLOR_ prefix)
  // NOTE: swapped numbering per request: Light1 <-> Light3
  if (cmd == "LIGHT1_WHITE") { uart.send("CMD:SEG3_WHITE"); return true; }
  if (cmd == "LIGHT1_RED") { uart.send("CMD:SEG3_RED"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHT1_GREEN") { uart.send("CMD:SEG3_GREEN"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHT1_BLUE") { uart.send("CMD:SEG3_BLUE"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHT1_YELLOW") { uart.send("CMD:SEG3_YELLOW"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHT1_PURPLE") { uart.send("CMD:SEG3_PURPLE"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }

  if (cmd == "LIGHT2_WHITE") { uart.send("CMD:SEG2_WHITE"); return true; }
  if (cmd == "LIGHT2_RED") { uart.send("CMD:SEG2_RED"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHT2_GREEN") { uart.send("CMD:SEG2_GREEN"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHT2_BLUE") { uart.send("CMD:SEG2_BLUE"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHT2_YELLOW") { uart.send("CMD:SEG2_YELLOW"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHT2_PURPLE") { uart.send("CMD:SEG2_PURPLE"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }

  if (cmd == "LIGHT3_WHITE") { uart.send("CMD:SEG1_WHITE"); return true; }
  if (cmd == "LIGHT3_RED") { uart.send("CMD:SEG1_RED"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHT3_GREEN") { uart.send("CMD:SEG1_GREEN"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHT3_BLUE") { uart.send("CMD:SEG1_BLUE"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHT3_YELLOW") { uart.send("CMD:SEG1_YELLOW"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHT3_PURPLE") { uart.send("CMD:SEG1_PURPLE"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }

  if (cmd == "LIGHTS_ALL_WHITE") { uart.send("CMD:ALL_WHITE"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHTS_ALL_RED") { uart.send("CMD:ALL_RED"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHTS_ALL_GREEN") { uart.send("CMD:ALL_GREEN"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHTS_ALL_BLUE") { uart.send("CMD:ALL_BLUE"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHTS_ALL_YELLOW") { uart.send("CMD:ALL_YELLOW"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }
  if (cmd == "LIGHTS_ALL_PURPLE") { uart.send("CMD:ALL_PURPLE"); if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN); return true; }

  if (cmd == "LIGHT1_ON" || cmd == "LED1_ON") { uart.send("CMD:SEG3_ON"); if (audioReady()) audioPlay(audio_tracks::BATDEN); return true; }
  if (cmd == "LIGHT1_OFF" || cmd == "LED1_OFF") { uart.send("CMD:SEG3_OFF"); if (audioReady()) audioPlay(audio_tracks::TATDEN); return true; }
  if (cmd == "LIGHT2_ON") { uart.send("CMD:SEG2_ON"); if (audioReady()) audioPlay(audio_tracks::BATDEN); return true; }
  if (cmd == "LIGHT2_OFF") { uart.send("CMD:SEG2_OFF"); if (audioReady()) audioPlay(audio_tracks::TATDEN); return true; }
  if (cmd == "LIGHT3_ON") { uart.send("CMD:SEG1_ON"); if (audioReady()) audioPlay(audio_tracks::BATDEN); return true; }
  if (cmd == "LIGHT3_OFF") { uart.send("CMD:SEG1_OFF"); if (audioReady()) audioPlay(audio_tracks::TATDEN); return true; }

  // Color commands: if RAINBOW -> rainbow, otherwise just turn segment(s) ON
  if (cmd.startsWith("LIGHT1_COLOR_")) {
    if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN);
    if (cmd.endsWith("RAINBOW")) uart.send("CMD:LED_RAINBOW");
    else if (cmd.endsWith("RED")) uart.send("CMD:SEG3_RED");
    else if (cmd.endsWith("GREEN")) uart.send("CMD:SEG3_GREEN");
    else if (cmd.endsWith("BLUE")) uart.send("CMD:SEG3_BLUE");
    else if (cmd.endsWith("YELLOW")) uart.send("CMD:SEG3_YELLOW");
    else if (cmd.endsWith("PURPLE")) uart.send("CMD:SEG3_PURPLE");
    else if (cmd.endsWith("WHITE")) uart.send("CMD:SEG3_WHITE");
    else uart.send("CMD:SEG3_ON");
    return true;
  }
  if (cmd.startsWith("LIGHT2_COLOR_")) {
    if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN);
    if (cmd.endsWith("RAINBOW")) uart.send("CMD:LED_RAINBOW");
    else if (cmd.endsWith("RED")) uart.send("CMD:SEG2_RED");
    else if (cmd.endsWith("GREEN")) uart.send("CMD:SEG2_GREEN");
    else if (cmd.endsWith("BLUE")) uart.send("CMD:SEG2_BLUE");
    else if (cmd.endsWith("YELLOW")) uart.send("CMD:SEG2_YELLOW");
    else if (cmd.endsWith("PURPLE")) uart.send("CMD:SEG2_PURPLE");
    else if (cmd.endsWith("WHITE")) uart.send("CMD:SEG2_WHITE");
    else uart.send("CMD:SEG2_ON");
    return true;
  }
  if (cmd.startsWith("LIGHT3_COLOR_")) {
    if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN);
    if (cmd.endsWith("RAINBOW")) uart.send("CMD:LED_RAINBOW");
    else if (cmd.endsWith("RED")) uart.send("CMD:SEG1_RED");
    else if (cmd.endsWith("GREEN")) uart.send("CMD:SEG1_GREEN");
    else if (cmd.endsWith("BLUE")) uart.send("CMD:SEG1_BLUE");
    else if (cmd.endsWith("YELLOW")) uart.send("CMD:SEG1_YELLOW");
    else if (cmd.endsWith("PURPLE")) uart.send("CMD:SEG1_PURPLE");
    else if (cmd.endsWith("WHITE")) uart.send("CMD:SEG1_WHITE");
    else uart.send("CMD:SEG1_ON");
    return true;
  }
  if (cmd.startsWith("LIGHTS_ALL_COLOR_")) {
    if (audioReady()) audioPlay(audio_tracks::DOIMAUDEN);
    if (cmd.endsWith("RAINBOW")) uart.send("CMD:LED_RAINBOW");
    else if (cmd.endsWith("RED")) uart.send("CMD:ALL_RED");
    else if (cmd.endsWith("GREEN")) uart.send("CMD:ALL_GREEN");
    else if (cmd.endsWith("BLUE")) uart.send("CMD:ALL_BLUE");
    else if (cmd.endsWith("YELLOW")) uart.send("CMD:ALL_YELLOW");
    else if (cmd.endsWith("PURPLE")) uart.send("CMD:ALL_PURPLE");
    else if (cmd.endsWith("WHITE")) uart.send("CMD:ALL_WHITE");
    else uart.send("CMD:LED_ALL");
    return true;
  }

  // RFID management
  if (cmd == "RFID_LIST") {
    // Emit cards list (bridge expects)
    StaticJsonDocument<768> doc;
    doc["type"] = "rfid_cards";
    doc["v"] = 1;
    doc["updated_at"] = isoMs_(millis());
    JsonArray arr = doc.createNestedArray("cards");
    for (int i = 0; i < g_cardCount; i++) arr.add(g_cards[i].uid);
    serialPrintJson_(doc);
    return true;
  }
  if (cmd.startsWith("RFID_ADD:")) {
    String rest = cmd.substring(9);
    // Allow format: RFID_ADD:<UID>|<NAME>
    String uid = rest;
    String name = "";
    const int sep = rest.indexOf('|');
    if (sep >= 0) {
      uid = rest.substring(0, sep);
      name = rest.substring(sep + 1);
    }
    uid.trim();
    uid.toUpperCase();
    if (!uidValid_(uid)) { errOut = "bad_uid"; return false; }
    if (g_cardCount >= MAX_RFID_CARDS) { errOut = "cards_full"; return false; }
    for (int i = 0; i < g_cardCount; i++) {
      if (uidEquals_(g_cards[i].uid, uid.c_str())) {
        // Update name if provided
        name = nameSanitize_(name);
        if (name.length() > 0) {
          memset(g_cards[i].name, 0, sizeof(g_cards[i].name));
          strncpy(g_cards[i].name, name.c_str(), sizeof(g_cards[i].name) - 1);
          saveCards_();
        }
        return true;
      }
    }
    memset(&g_cards[g_cardCount], 0, sizeof(RfidEntry));
    strncpy(g_cards[g_cardCount].uid, uid.c_str(), sizeof(g_cards[g_cardCount].uid) - 1);
    name = nameSanitize_(name);
    strncpy(g_cards[g_cardCount].name, name.c_str(), sizeof(g_cards[g_cardCount].name) - 1);
    g_cards[g_cardCount].inside = false;
    g_cardCount++;
    saveCards_();
    return true;
  }
  if (cmd.startsWith("RFID_DEL:")) {
    String uid = cmd.substring(9);
    uid.trim();
    uid.toUpperCase();
    if (!uidValid_(uid)) { errOut = "bad_uid"; return false; }
    for (int i = 0; i < g_cardCount; i++) {
      if (uidEquals_(g_cards[i].uid, uid.c_str())) {
        for (int j = i + 1; j < g_cardCount; j++) g_cards[j - 1] = g_cards[j];
        g_cardCount--;
        saveCards_();
        return true;
      }
    }
    return true;
  }
  if (cmd.startsWith("RFID_SCAN:")) {
    String uid = cmd.substring(10);
    uid.trim();
    uid.toUpperCase();
    if (!uidValid_(uid)) { errOut = "bad_uid"; return false; }
    const bool ok = isAuthorized_(uid.c_str());
    pushHistory_(uid.c_str(), ok);
    emitRfidScanEvent_(uid.c_str(), ok, "RFID");
    if (ok) {
      // Door action based on direction we just computed in pushHistory_ (stored in g_lastScan)
      if (strcmp(g_lastScan.direction, "IN") == 0) handleDoorOpen_("RFID");
      else if (strcmp(g_lastScan.direction, "OUT") == 0) handleDoorClose_("RFID");
      else handleDoorOpen_("RFID");
    }
    return true;
  }

  errOut = "unknown_cmd";
  return false;
}

static void emitCapsOnce_() {
  if (g_capsSent) return;
  g_capsSent = true;
  Serial.println(gatewayBuildCapsJson());
}

// USB serial line reader
static char g_rx[160];
static size_t g_rxIdx = 0;

static void processSerialLine_(const char* line) {
  String s(line);
  s.trim();
  if (s.length() == 0) return;

  // JSON lines mode (preferred)
  if (s[0] == '{') {
    StaticJsonDocument<256> in;
    DeserializationError e = deserializeJson(in, s);
    if (!e) {
      const char* type = in["type"] | "";
      const char* cmdC = in["cmd"] | "";
      const uint32_t id = in["id"] | g_seq++;

      if ((strcmp(type, "cmd") == 0 || strcmp(type, "control") == 0) && cmdC[0] != '\0') {
        String cmd(cmdC);
        String err;
        const bool ok = gatewayHandleCommand(cmd, err, "SERIAL");
        emitAck_(id, cmd, ok, ok ? nullptr : err.c_str());
      }
      return;
    }
    // fallthrough to plain text if JSON parse fails
  }

  // Plain text mode: one command per line
  const uint32_t id = g_seq++;
  String err;
  const bool ok = gatewayHandleCommand(s, err, "SERIAL");
  emitAck_(id, s, ok, ok ? nullptr : err.c_str());
}

} // namespace

void gatewayBegin() {
  memset(&g_lastScan, 0, sizeof(g_lastScan));
  g_hasLast = false;
  g_histCount = 0;
  g_cardCount = 0;
  g_pinCount = 0;
  g_epinCount = 0;
  g_lastStateMs = 0;
  g_seq = 1;
  g_capsSent = false;

  g_hostageActive = false;
  g_hostageStartMs = 0;
  g_hostageNextBeepMs = 0;
  g_hostageEscalated = false;

  loadCards_();
  loadPins_();
  loadEmergencyPins_();
}

bool gatewayHandleCommand(const String& cmd, String& errOut, const char* source) {
  return handleCommandInternal_(cmd, errOut, source);
}

void gatewayRfidRecordScan(const char* uid, bool authorized, const char* name, const char* direction, long epoch) {
  if (!uid || uid[0] == '\0') return;
  pushHistoryDirect_(uid, authorized, name, direction, epoch);
}

void gatewaySerialLoop() {
  emitCapsOnce_();

  while (Serial.available()) {
    const char c = (char)Serial.read();

    if (c == '\r' || c == '\n') {
      if (g_rxIdx > 0) {
        g_rx[g_rxIdx] = '\0';
        processSerialLine_(g_rx);
        g_rxIdx = 0;
      }
      continue;
    }

    if (g_rxIdx < sizeof(g_rx) - 1) {
      g_rx[g_rxIdx++] = c;
    } else {
      // overflow -> reset
      g_rxIdx = 0;
    }
  }
}

void gatewayRfidLoop() {
  char uid[24];
  if (rfid.pollUID(uid, sizeof(uid))) {
    // UI feedback on Mega LCD
    uart.send("CMD:RFID_CHECK");

    const bool ok = isAuthorized_(uid);
    pushHistory_(uid, ok);
    emitRfidScanEvent_(uid, ok, "RFID");

    if (ok) {
      // Show name + direction; still open the door for both IN/OUT.
      const char* nm = findNameByUid_(uid);
      const char* dir = g_lastScan.direction;
      String hello = String("CMD:HELLO:")
        + String((nm && nm[0]) ? nm : "Ban")
        + "|"
        + String((dir && dir[0]) ? dir : "IN");
      uart.send(hello);
    } else {
      uart.send("CMD:RFID_FAIL");
    }

    if (ok) {
      // Always open on authorized scan. Door will auto-close by existing timer.
      handleDoorOpen_("RFID");
    }
  }
}

String gatewayBuildSensorsJson(bool includeType) {
  StaticJsonDocument<1536> doc;
  if (includeType) {
    doc["type"] = "state";
    doc["v"] = 1;
    doc["seq"] = g_seq++;
  }

  // Curtain motion: -1 closing, 0 idle, 1 opening
  doc["curtain_motion"] = shadow.curtain;
  doc["curtain"] = shadow.curtain; // alias
  doc["CUR"] = shadow.curtain;     // alias (telemetry-style)
  if (shadow.curtain > 0) doc["curtain_state"] = "OPENING";
  else if (shadow.curtain < 0) doc["curtain_state"] = "CLOSING";
  else doc["curtain_state"] = "STOP";

  if (isnan(shadow.tempC)) doc["temperature_c"] = nullptr;
  else doc["temperature_c"] = shadow.tempC;

  if (isnan(shadow.humidPct)) doc["humidity_percent"] = nullptr;
  else doc["humidity_percent"] = shadow.humidPct;
  doc["gas_value"] = shadow.gas;
  doc["fire_status"] = (shadow.fire != 0) ? "FIRE" : "Safe";

  // Extra sensors for web UI (realtime)
  doc["ultrasonic_cm"] = shadow.usCm;
  doc["ultrasonic_valid"] = shadow.usOk && (shadow.usCm >= 0);
  doc["ultra_cm"] = shadow.usCm;
  doc["distance_cm"] = shadow.usCm;
  doc["ldr_raw"] = shadow.ldr;
  doc["ldr_is_dark"] = shadow.ldrDark;
  doc["ldr_threshold"] = shadow.ldrThreshold;

  // Backward-compatible keys (some web UIs expect shorter names)
  doc["us"] = shadow.usCm;
  doc["us_ok"] = shadow.usOk;
  doc["ldr"] = shadow.ldr;
  doc["dark"] = shadow.ldrDark;
  doc["ldr_th"] = shadow.ldrThreshold;

  // Compatibility with telemetry-style keys
  doc["US"] = shadow.usCm;
  doc["USOK"] = shadow.usOk;
  doc["LDR"] = shadow.ldr;
  doc["DARK"] = shadow.ldrDark;
  doc["LTH"] = shadow.ldrThreshold;

  // Emergency/hostage password state
  doc["hostage"] = g_hostageActive;
  doc["hostage_active"] = g_hostageActive;
  doc["hostage_siren"] = g_hostageEscalated;
  doc["emergency_password"] = g_hostageActive;
  doc["emergency_password_siren"] = g_hostageEscalated;
  // Compatibility aliases (some dashboards use camelCase)
  doc["hostageActive"] = g_hostageActive;
  doc["hostageEscalated"] = g_hostageEscalated;
  doc["emergencyPassword"] = g_hostageActive;
  doc["emergencyPasswordSiren"] = g_hostageEscalated;
  if (g_hostageActive) {
    doc["emergency_message"] = g_hostageEscalated
      ? "CANH BAO: DANG BAO DONG"
      : "CANH BAO: MAT KHAU KHAN CAP";
    doc["emergencyMessage"] = doc["emergency_message"];
  }

  // Someone standing at door (<=15cm for >30s; computed by Mega)
  doc["door_presence"] = shadow.pres;
  doc["presence_at_door"] = shadow.pres;
  if (shadow.pres) {
    doc["presence_message"] = "Phat hien co nguoi dung truoc cua";
  }

  // Door state (top-level aliases for web dashboards)
  doc["door_open"] = shadow.doorOpen;
  doc["door"] = shadow.doorOpen;
  doc["DOOR"] = shadow.doorOpen ? 1 : 0;
  doc["door_state"] = shadow.doorOpen ? "OPEN" : "CLOSED";
  doc["door_locked"] = shadow.locked;

  doc["locked"] = shadow.locked;
  doc["LOCK"] = shadow.locked ? 1 : 0;
  doc["door_raw"] = shadow.doorRaw;
  doc["door_open_level"] = shadow.doorPol;

  // Audio diagnostics
  doc["audio_ready"] = audioReady();

  doc["updated_at"] = isoMs_(millis());

  JsonObject dev = doc.createNestedObject("devices");
  dev["fan1"] = shadow.fan1;
  dev["fan2"] = shadow.fan2;
  dev["door_open"] = shadow.doorOpen;
  dev["door_last_method"] = g_lastDoorMethod;
  dev["door_locked"] = shadow.locked;
  dev["hostage"] = g_hostageActive;
  dev["hostage_siren"] = g_hostageEscalated;
  dev["curtain_motion"] = shadow.curtain;
  dev["curtain"] = shadow.curtain; // alias
  dev["CUR"] = shadow.curtain;     // alias

  // Map Mega LED segments to 3 lights
  const bool hw1 = (shadow.ledMask & 0x01) != 0; // Mega SEG1
  const bool hw2 = (shadow.ledMask & 0x02) != 0; // Mega SEG2
  const bool hw3 = (shadow.ledMask & 0x04) != 0; // Mega SEG3

  // NOTE: swapped numbering per request: Light1 <-> Light3
  const bool l1 = hw3;
  const bool l2 = hw2;
  const bool l3 = hw1;

  dev["light1"] = l1;
  dev["light2"] = l2;
  dev["light3"] = l3;

  dev["light1_color"] = l1 ? colorIdToStr_(shadow.lc3) : "OFF";
  dev["light2_color"] = l2 ? colorIdToStr_(shadow.lc2) : "OFF";
  dev["light3_color"] = l3 ? colorIdToStr_(shadow.lc1) : "OFF";

  if (shadow.ledRainbow) {
    dev["lights_all_color"] = "RAINBOW";
  } else if (!l1 && !l2 && !l3) {
    dev["lights_all_color"] = "OFF";
  } else if (l1 && l2 && l3 && shadow.lc1 == shadow.lc2 && shadow.lc2 == shadow.lc3) {
    dev["lights_all_color"] = colorIdToStr_(shadow.lc1);
  } else {
    dev["lights_all_color"] = "MIXED";
  }

  String out;
  serializeJson(doc, out);
  return out;
}

String gatewayBuildCapsJson() {
  StaticJsonDocument<512> doc;
  doc["type"] = "caps";
  doc["v"] = 1;
  doc["fw"] = "firmware_esp32";
  JsonObject features = doc.createNestedObject("features");
  features["mega_uart"] = true;
  features["rfid"] = true;
  features["http"] = true;
  features["serial_json"] = true;

  JsonObject pins = doc.createNestedObject("pins");
  pins["mega_uart_rx"] = 16;
  pins["mega_uart_tx"] = 17;
  pins["rc522_ss"] = 5;
  pins["rc522_rst"] = 4;
  pins["dfp_rx"] = 32;
  pins["dfp_tx"] = 33;

  String out;
  serializeJson(doc, out);
  return out;
}

String gatewayBuildRfidLastJson() {
  StaticJsonDocument<384> doc;
  doc["uid"] = g_hasLast ? g_lastScan.uid : "";
  doc["name"] = g_hasLast ? g_lastScan.name : "";
  doc["authorized"] = g_hasLast ? g_lastScan.authorized : false;
  doc["direction"] = g_hasLast ? g_lastScan.direction : "";
  doc["time_ms"] = g_hasLast ? (unsigned long)g_lastScan.tsMs : 0UL;
  doc["time_epoch"] = g_hasLast ? g_lastScan.epoch : 0L;
  if (g_hasLast && g_lastScan.epoch > 0) {
    time_t t = (time_t)g_lastScan.epoch;
    struct tm tmv;
    localtime_r(&t, &tmv);
    char buf[32];
    // Local time ISO-like: YYYY-MM-DD HH:MM:SS
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    doc["time_iso"] = buf;
  } else {
    doc["time_iso"] = "";
  }
  doc["updated_at"] = isoMs_(millis());
  String out;
  serializeJson(doc, out);
  return out;
}

String gatewayBuildRfidCardsJson() {
  StaticJsonDocument<1536> doc;
  // Backward compatible: keep `cards` as array of UID strings.
  JsonArray cards = doc.createNestedArray("cards");
  for (int i = 0; i < g_cardCount; i++) cards.add(g_cards[i].uid);

  // New: `cards_named` provides uid+name for web UI.
  JsonArray named = doc.createNestedArray("cards_named");
  for (int i = 0; i < g_cardCount; i++) {
    JsonObject o = named.createNestedObject();
    o["uid"] = g_cards[i].uid;
    o["name"] = g_cards[i].name;
  }
  doc["updated_at"] = isoMs_(millis());
  String out;
  serializeJson(doc, out);
  return out;
}

String gatewayBuildRfidHistoryJson() {
  StaticJsonDocument<2048> doc;
  JsonArray hist = doc.createNestedArray("history");
  for (int i = 0; i < g_histCount; i++) {
    JsonObject o = hist.createNestedObject();
    o["uid"] = g_history[i].uid;
    o["name"] = g_history[i].name;
    o["authorized"] = g_history[i].authorized;
    o["direction"] = g_history[i].direction;
    o["time_ms"] = (unsigned long)g_history[i].tsMs;
    o["time_epoch"] = g_history[i].epoch;

    if (g_history[i].epoch > 0) {
      time_t t = (time_t)g_history[i].epoch;
      struct tm tmv;
      localtime_r(&t, &tmv);
      char buf[32];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
      o["time_iso"] = buf;
    } else {
      o["time_iso"] = "";
    }
  }
  doc["updated_at"] = isoMs_(millis());
  String out;
  serializeJson(doc, out);
  return out;
}

String gatewayBuildPinsJson() {
  StaticJsonDocument<384> doc;
  JsonArray pins = doc.createNestedArray("pins");
  for (int i = 0; i < g_pinCount; i++) pins.add(g_pins[i].pin);
  doc["updated_at"] = isoMs_(millis());
  String out;
  serializeJson(doc, out);
  return out;
}

String gatewayBuildEmergencyPinsJson() {
  StaticJsonDocument<256> doc;
  doc["type"] = "pins_emergency";
  doc["v"] = 1;
  doc["count"] = g_epinCount;
  JsonArray arr = doc.createNestedArray("pins");
  // Don't expose emergency PINs over HTTP.
  for (int i = 0; i < g_epinCount; i++) arr.add("****");
  doc["updated_at"] = isoMs_(millis());
  String out;
  serializeJson(doc, out);
  return out;
}

bool gatewayRfidAddCard(const String& uidIn, const String& nameIn) {
  String uid = uidIn;
  uid.trim();
  uid.toUpperCase();
  if (!uidValid_(uid)) return false;
  if (g_cardCount >= MAX_RFID_CARDS) return false;

  String name = nameSanitize_(nameIn);
  for (int i = 0; i < g_cardCount; i++) {
    if (uidEquals_(g_cards[i].uid, uid.c_str())) {
      if (name.length() > 0) {
        memset(g_cards[i].name, 0, sizeof(g_cards[i].name));
        strncpy(g_cards[i].name, name.c_str(), sizeof(g_cards[i].name) - 1);
        saveCards_();
      }
      return true;
    }
  }
  memset(&g_cards[g_cardCount], 0, sizeof(RfidEntry));
  strncpy(g_cards[g_cardCount].uid, uid.c_str(), sizeof(g_cards[g_cardCount].uid) - 1);
  strncpy(g_cards[g_cardCount].name, name.c_str(), sizeof(g_cards[g_cardCount].name) - 1);
  g_cards[g_cardCount].inside = false;
  g_cardCount++;
  saveCards_();
  return true;
}

bool gatewayRfidDelCard(const String& uidIn) {
  String uid = uidIn;
  uid.trim();
  uid.toUpperCase();
  if (!uidValid_(uid)) return false;
  for (int i = 0; i < g_cardCount; i++) {
    if (uidEquals_(g_cards[i].uid, uid.c_str())) {
      for (int j = i + 1; j < g_cardCount; j++) g_cards[j - 1] = g_cards[j];
      g_cardCount--;
      saveCards_();
      return true;
    }
  }
  return true;
}

bool gatewayPinAdd(const String& pinIn) {
  String pin = pinIn;
  pin.trim();
  if (!pinValid_(pin)) return false;
  if (g_pinCount >= MAX_PINS) return false;
  for (int i = 0; i < g_pinCount; i++) {
    if (pinEquals_(g_pins[i].pin, pin.c_str())) return true;
  }
  memset(&g_pins[g_pinCount], 0, sizeof(PinEntry));
  strncpy(g_pins[g_pinCount].pin, pin.c_str(), sizeof(g_pins[g_pinCount].pin) - 1);
  g_pinCount++;
  savePins_();
  return true;
}

bool gatewayPinDel(const String& pinIn) {
  String pin = pinIn;
  pin.trim();
  if (!pinValid_(pin)) return false;
  for (int i = 0; i < g_pinCount; i++) {
    if (pinEquals_(g_pins[i].pin, pin.c_str())) {
      for (int j = i + 1; j < g_pinCount; j++) g_pins[j - 1] = g_pins[j];
      g_pinCount--;
      savePins_();
      return true;
    }
  }
  return true;
}

bool gatewayEmergencyPinAdd(const String& pinIn) {
  String pin = pinIn;
  pin.trim();
  if (!pinValid_(pin)) return false;
  if (g_epinCount >= MAX_EMERGENCY_PINS) return false;
  for (int i = 0; i < g_epinCount; i++) {
    if (pinEquals_(g_epins[i].pin, pin.c_str())) return true;
  }
  memset(&g_epins[g_epinCount], 0, sizeof(EmergencyPinEntry));
  strncpy(g_epins[g_epinCount].pin, pin.c_str(), sizeof(g_epins[g_epinCount].pin) - 1);
  g_epinCount++;
  saveEmergencyPins_();
  return true;
}

bool gatewayEmergencyPinDel(const String& pinIn) {
  String pin = pinIn;
  pin.trim();
  if (!pinValid_(pin)) return false;
  for (int i = 0; i < g_epinCount; i++) {
    if (pinEquals_(g_epins[i].pin, pin.c_str())) {
      for (int j = i + 1; j < g_epinCount; j++) g_epins[j - 1] = g_epins[j];
      g_epinCount--;
      saveEmergencyPins_();
      return true;
    }
  }
  return true;
}

bool gatewayGetLastRfid(GatewayRfidLast& out) {
  memset(&out, 0, sizeof(out));
  if (!g_hasLast) return false;
  strncpy(out.uid, g_lastScan.uid, sizeof(out.uid) - 1);
  strncpy(out.name, g_lastScan.name, sizeof(out.name) - 1);
  out.authorized = g_lastScan.authorized;
  strncpy(out.direction, g_lastScan.direction, sizeof(out.direction) - 1);
  out.tsMs = g_lastScan.tsMs;
  out.epoch = g_lastScan.epoch;
  return true;
}

void gatewayStateLoop() {
  const unsigned long now = millis();
  if (now - g_lastStateMs < 1000UL) return;
  g_lastStateMs = now;

  // Hostage escalation:
  // - first 20s: small beep every 5s (Mega buzzer)
  // - after 20s: continuous alarm + play MP3 0022 once
  if (g_hostageActive) {
    const unsigned long elapsed = now - g_hostageStartMs;
    if (elapsed < 20000UL) {
      if ((long)(now - g_hostageNextBeepMs) >= 0) {
        uart.send("CMD:HOSTAGE_BEEP");
        g_hostageNextBeepMs += 5000UL;
      }
    } else if (!g_hostageEscalated) {
      uart.send("CMD:HOSTAGE_SIREN");
      if (audioReady()) audioPlay(audio_tracks::BAODONGDEDOA);
      g_hostageEscalated = true;
    }
  }

  // Emit exactly what bridge expects: JSON line with type=="state"
  Serial.println(gatewayBuildSensorsJson(true));
}
