#include "src/core/config.h"
#include "src/comm/uart_link.h"
#include "src/comm/protocol_parser.h"
#include "src/devices/state_sync.h"
#include "src/devices/device_shadow.h"
#include "src/devices/rfid_mgr.h"
#include "src/devices/lcd2004_mgr.h"
#include "src/devices/dfplayer_mgr.h"
#include "src/devices/audio_mgr.h"
#include "src/devices/audio_tracks.h"
#include "src/cloud/google_sheet.h"
#include "src/network/web_server.h"
#include "src/health/system_monitor.h"
#include "src/gateway/gateway.h"

#include <esp_system.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <time.h>

UARTLink uart;
RFIDMgr rfid;
LCD2004Mgr lcd;
DFPlayerMgr mp3;
GoogleSheetLogger gsheet;

namespace {
static bool g_bootAudioPending = false;
static unsigned long g_bootAudioAtMs = 0;
static bool g_bootAudioPlayed = false;
static uint8_t g_bootAudioAttempts = 0;
static uint8_t g_bootAudioStage = 0; // 0=waiting to start, 1=welcome playing, 2=done

static const unsigned long BOOT_WELCOME_DELAY_MS = 2800UL; // adjust to your 0001.mp3 length if needed

static bool g_prevEmergency = false;
static bool g_prevGasDanger = false;
static bool g_prevFire = false;
static bool g_prevLdrDark = false;
static bool g_prevTempHigh = false;
static bool g_prevPresence = false;
}

namespace {

static Preferences g_scanPrefs;
static bool g_scanPrefsStarted = false;

struct InsideEntry {
  char uid[24];
  bool inside;
};

static const int MAX_INSIDE_ENTRIES = 40;
static InsideEntry g_inside[MAX_INSIDE_ENTRIES];
static int g_insideCount = 0;

struct PendingScan {
  char uid[24];
  char direction[8];
  bool nextInside;
  long epoch;
  unsigned long createdMs;
};

static const int MAX_PENDING = 6;
static PendingScan g_pending[MAX_PENDING];
static int g_pendingCount = 0;

static void scanPrefsEnsure_() {
  if (g_scanPrefsStarted) return;
  g_scanPrefsStarted = true;
  (void)g_scanPrefs.begin("miki_scan", false);
}

static long nowEpoch_() {
  const time_t now = time(nullptr);
  if (now < 1700000000L) return 0;
  return (long)now;
}

static void loadInside_() {
  scanPrefsEnsure_();
  g_insideCount = 0;
  const String json = g_scanPrefs.getString("inside", "");
  if (json.length() == 0) return;

  StaticJsonDocument<2048> doc;
  if (deserializeJson(doc, json)) return;
  JsonArray arr = doc["inside"].as<JsonArray>();
  if (arr.isNull()) return;

  for (JsonVariant v : arr) {
    const char* uidC = v["uid"] | "";
    const bool inside = v["inside"] | false;
    if (!uidC || uidC[0] == '\0') continue;
    if (g_insideCount >= MAX_INSIDE_ENTRIES) break;
    memset(&g_inside[g_insideCount], 0, sizeof(InsideEntry));
    strncpy(g_inside[g_insideCount].uid, uidC, sizeof(g_inside[g_insideCount].uid) - 1);
    g_inside[g_insideCount].inside = inside;
    g_insideCount++;
  }
}

static void saveInside_() {
  scanPrefsEnsure_();
  StaticJsonDocument<2048> doc;
  JsonArray arr = doc.createNestedArray("inside");
  for (int i = 0; i < g_insideCount; i++) {
    JsonObject o = arr.createNestedObject();
    o["uid"] = g_inside[i].uid;
    o["inside"] = g_inside[i].inside;
  }
  String out;
  serializeJson(doc, out);
  (void)g_scanPrefs.putString("inside", out);
}

static void setInsideByUid_(const char* uid, bool inside) {
  for (int i = 0; i < g_insideCount; i++) {
    if (strcmp(g_inside[i].uid, uid) == 0) {
      g_inside[i].inside = inside;
      saveInside_();
      return;
    }
  }

  if (g_insideCount >= MAX_INSIDE_ENTRIES) {
    for (int i = 1; i < g_insideCount; i++) g_inside[i - 1] = g_inside[i];
    g_insideCount = MAX_INSIDE_ENTRIES - 1;
  }
  memset(&g_inside[g_insideCount], 0, sizeof(InsideEntry));
  strncpy(g_inside[g_insideCount].uid, uid, sizeof(g_inside[g_insideCount].uid) - 1);
  g_inside[g_insideCount].inside = inside;
  g_insideCount++;
  saveInside_();
}

static bool getInsideByUid_(const char* uid, bool& insideOut) {
  for (int i = 0; i < g_insideCount; i++) {
    if (strcmp(g_inside[i].uid, uid) == 0) {
      insideOut = g_inside[i].inside;
      return true;
    }
  }
  return false;
}

static void pendingAdd_(const char* uid, const char* direction, bool nextInside, long epoch) {
  for (int i = 0; i < g_pendingCount; i++) {
    if (strcmp(g_pending[i].uid, uid) == 0) {
      strncpy(g_pending[i].direction, direction, sizeof(g_pending[i].direction) - 1);
      g_pending[i].nextInside = nextInside;
      g_pending[i].epoch = epoch;
      g_pending[i].createdMs = millis();
      return;
    }
  }

  if (g_pendingCount >= MAX_PENDING) {
    int oldest = 0;
    for (int i = 1; i < g_pendingCount; i++) {
      if (g_pending[i].createdMs < g_pending[oldest].createdMs) oldest = i;
    }
    g_pending[oldest] = g_pending[g_pendingCount - 1];
    g_pendingCount--;
  }

  memset(&g_pending[g_pendingCount], 0, sizeof(PendingScan));
  strncpy(g_pending[g_pendingCount].uid, uid, sizeof(g_pending[g_pendingCount].uid) - 1);
  strncpy(g_pending[g_pendingCount].direction, direction, sizeof(g_pending[g_pendingCount].direction) - 1);
  g_pending[g_pendingCount].nextInside = nextInside;
  g_pending[g_pendingCount].epoch = epoch;
  g_pending[g_pendingCount].createdMs = millis();
  g_pendingCount++;
}

static bool pendingTake_(const char* uid, PendingScan& out) {
  for (int i = 0; i < g_pendingCount; i++) {
    if (strcmp(g_pending[i].uid, uid) == 0) {
      out = g_pending[i];
      g_pending[i] = g_pending[g_pendingCount - 1];
      g_pendingCount--;
      return true;
    }
  }
  return false;
}

} // namespace

static const char* resetReasonToStr(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_UNKNOWN: return "UNKNOWN";
    case ESP_RST_POWERON: return "POWERON";
    case ESP_RST_EXT: return "EXT";
    case ESP_RST_SW: return "SW";
    case ESP_RST_PANIC: return "PANIC";
    case ESP_RST_INT_WDT: return "INT_WDT";
    case ESP_RST_TASK_WDT: return "TASK_WDT";
    case ESP_RST_WDT: return "WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT: return "BROWNOUT";
    case ESP_RST_SDIO: return "SDIO";
    default: return "(other)";
  }
}

static void renderDashboard() {
  const bool megaOnline = (shadow.lastRxMs != 0) && (millis() - shadow.lastRxMs < 3500UL);
  const bool wifiOk = (WiFi.status() == WL_CONNECTED);

  static uint8_t page = 0;
  static unsigned long lastFlipMs = 0;
  if (millis() - lastFlipMs > 2500UL) {
    lastFlipMs = millis();
    page = (page + 1) % 2;
  }

  {
    String h = String("MEGA:") + (megaOnline ? "OK " : "-- ") + "WIFI:" + (wifiOk ? "OK" : "--");
    lcd.printLine(0, h);
  }

  if (!megaOnline) {
    lcd.printLine(1, "NO DATA FROM MEGA");
    lcd.printLine(2, "Wiring: GND+UART");
    lcd.printLine(3, "ESP RX16 TX17");
    return;
  }

  if (page == 0) {
    String l1 = String("MODE ") + shadow.mode + (shadow.emergency ? " !EMG" : "");
    lcd.printLine(1, l1);

    String l2 = String("T") + String(shadow.tempC, 1) + "C H" + String(shadow.humidPct, 1) + "%";
    lcd.printLine(2, l2);

    String l3 = String("G") + shadow.gas + "/" + shadow.gasThreshold + " F" + shadow.fire + " IR" + shadow.ir;
    lcd.printLine(3, l3);
  } else {
    String l1 = String("F1") + (shadow.fan1 ? "+" : "-") + " F2" + (shadow.fan2 ? "+" : "-") + " DO" + (shadow.doorOpen ? "O" : "C") + " LK" + (shadow.locked ? "1" : "0");
    lcd.printLine(1, l1);

    String l2 = String("CUR ") + shadow.curtain + " LED " + (shadow.ledRainbow ? "RBW" : "SEG");
    lcd.printLine(2, l2);

    String l3;
    if (wifiOk) {
      l3 = String("IP ") + WiFi.localIP().toString();
    } else {
      // Fallback hint if phone hotspot blocks LAN or STA is down.
      l3 = String("AP ") + WiFi.softAPIP().toString();
    }
    lcd.printLine(3, l3);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  auto reason = esp_reset_reason();
  Serial.printf("[BOOT] reset reason: %d (%s)\r\n", (int)reason, resetReasonToStr(reason));
  Serial.println("[BOOT] setup begin");

  gatewayBegin();

  lcd.begin();
  lcd.printLine(0, "MIKI SYSTEM");
  lcd.printLine(1, String("RST: ") + resetReasonToStr(reason));

  mp3.begin();
  audioBegin(mp3);
  if (mp3.ready()) {
    mp3.setVolume(30); // max
  }

  rfid.begin();
  gsheet.begin();
  loadInside_();

  uart.begin();
  Serial.println("[BOOT] uart begin ok");

  startWeb();
  Serial.println("[BOOT] web start ok");

  // Boot audio: play a few seconds after startup (DFPlayer needs time).
  g_bootAudioPending = true;
  g_bootAudioAtMs = millis() + 5500UL;
  g_bootAudioStage = 0;

  lcd.printLine(2, "Ready scan card");
}

void loop() {
  gatewaySerialLoop();
  webLoop();

  // DFPlayer recovery: if init failed at boot (SD slow, wiring glitch), retry periodically.
  {
    static unsigned long lastDfpRetryMs = 0;
    const unsigned long nowMs = millis();
    if (!mp3.ready() && (nowMs - lastDfpRetryMs) > 15000UL) {
      lastDfpRetryMs = nowMs;
      Serial.println("[DFP] retry begin (not ready)");
      mp3.begin();
      audioBegin(mp3);
      if (mp3.ready()) {
        mp3.setVolume(30);
        Serial.println("[DFP] ready after retry");
      }
    }
  }

  // Presence voice prompt (someone standing at door for >30s)
  if (!g_bootAudioPending) {
    const bool pres = shadow.pres;
    if (pres && !g_prevPresence) {
      if (audioReady()) audioPlay(audio_tracks::CNGUOI_TRUOC_CUA);
    }
    g_prevPresence = pres;
  }

  if (g_bootAudioPending && !g_bootAudioPlayed) {
    const unsigned long now = millis();

    if (g_bootAudioStage == 0) {
      if (now >= g_bootAudioAtMs) {
        if (audioReady()) {
          // Say 0001 first.
          audioPlay(audio_tracks::CHAOMUNG);
          g_bootAudioStage = 1;
          g_bootAudioAtMs = now + BOOT_WELCOME_DELAY_MS;
        } else {
          g_bootAudioAttempts++;
          Serial.printf("[BOOT] audio not ready (DFPlayer), retry=%u\r\n", (unsigned)g_bootAudioAttempts);
          // retry every 1s, for up to ~20s total
          if (g_bootAudioAttempts >= 20) {
            g_bootAudioPending = false;
          } else {
            g_bootAudioAtMs = now + 1000UL;
          }
        }
      }
    } else if (g_bootAudioStage == 1) {
      if (now >= g_bootAudioAtMs) {
        if (audioReady()) {
          const bool megaOnline = (shadow.lastRxMs != 0) && (now - shadow.lastRxMs < 3500UL);
          const bool wifiOk = (WiFi.status() == WL_CONNECTED);
          // Then say 0014 if stable else 0013.
          if (wifiOk && megaOnline) audioPlay(audio_tracks::KHOIDONGTHANHCONG);
          else audioPlay(audio_tracks::KHOIDONGKHONGTHANHCONG);
          g_bootAudioPlayed = true;
          g_bootAudioPending = false;
          g_bootAudioStage = 2;
        } else {
          // DFPlayer dropped? try again shortly.
          g_bootAudioAtMs = now + 500UL;
        }
      }
    }
  }

  // Telemetry-driven audio (gas/fire/emergency/LDR/temp)
  // Avoid interrupting boot audio sequence.
  if (!g_bootAudioPending) {
  {
    const bool gasDanger = (shadow.gasThreshold > 0) && (shadow.gas >= shadow.gasThreshold);
    const bool fire = (shadow.fire != 0);
    const bool emergency = shadow.emergency;

    // Fire edge
    if (fire && !g_prevFire) {
      if (audioReady()) audioPlay(audio_tracks::LUA);
    }

    // Gas edge
    if (gasDanger && !g_prevGasDanger) {
      if (audioReady()) audioPlay(audio_tracks::KHIGAS);
    }

    // Emergency cleared -> system stable audio (only when both dangers are clear)
    if (!emergency && g_prevEmergency && !gasDanger && !fire) {
      if (audioReady()) audioPlay(audio_tracks::TATKHANCAP);
    }

    g_prevFire = fire;
    g_prevGasDanger = gasDanger;
    g_prevEmergency = emergency;
  }

  // LDR edge (dark/bright) -> sounds
  {
    const bool dark = shadow.ldrDark;
    if (dark != g_prevLdrDark) {
      if (audioReady()) {
        if (dark) audioPlay(audio_tracks::QUANGTROBAT);
        else audioPlay(audio_tracks::QUANGTROTAT);
      }
      g_prevLdrDark = dark;
    }
  }

  // Temp >= 30C -> auto fan message (edge)
  {
    const bool high = !isnan(shadow.tempC) && (shadow.tempC >= 30.0f);
    if (high && !g_prevTempHigh) {
      if (audioReady()) audioPlay(audio_tracks::QUATTUDONG);
    }
    if (!high && g_prevTempHigh && shadow.tempC <= 29.0f) {
      // reset hysteresis
      g_prevTempHigh = false;
    } else {
      g_prevTempHigh = high;
    }
  }
  }

  String packet;
  while (uart.tryReadPacket(packet)) {
    // Mega -> ESP32 commands (keypad/admin)
    if (packet.startsWith("CMD:")) {
      const String cmd = packet.substring(4);
      String err;
      (void)gatewayHandleCommand(cmd, err, "MEGA");
      continue;
    }

    // Telemetry key/value (KEY:VALUE)
    ParsedMsg msg;
    if (parsePacket(packet, msg)) {
      syncFromMega(msg);

      // Door audio on physical transitions (including Mega auto-close)
      if (!g_bootAudioPending && msg.key == "DOOR") {
        static bool s_prevDoorKnown = false;
        static bool s_prevDoorOpen = false;
        static unsigned long s_lastDoorAudioMs = 0;

        const bool nowDoorOpen = shadow.doorOpen;
        if (!s_prevDoorKnown) {
          s_prevDoorKnown = true;
          s_prevDoorOpen = nowDoorOpen;
        } else {
          const unsigned long nowMs = millis();

          // Small cooldown avoids double-play if DOOR chatters.
          const bool canPlay = (nowMs - s_lastDoorAudioMs) > 600UL;

          if (canPlay) {
            if (!s_prevDoorOpen && nowDoorOpen) {
              // Suppress open-door voice during sensor emergency (gas/fire).
              // De-dup across command/event/edge using shared shadow timestamps.
              const bool voiceCooldownOk = (nowMs - shadow.lastDoorOpenVoiceMs) > 2500UL;
              if (!shadow.emergency && voiceCooldownOk && audioReady()) {
                audioPlay(audio_tracks::MOCUA);
                shadow.lastDoorOpenVoiceMs = nowMs;
                s_lastDoorAudioMs = nowMs;
              }
            } else if (s_prevDoorOpen && !nowDoorOpen) {
              const bool voiceCooldownOk = (nowMs - shadow.lastDoorCloseVoiceMs) > 2500UL;
              if (voiceCooldownOk && audioReady()) {
                audioPlay(audio_tracks::DONGCUA);
                shadow.lastDoorCloseVoiceMs = nowMs;
                s_lastDoorAudioMs = nowMs;
              }
            }
          }
          s_prevDoorOpen = nowDoorOpen;
        }
      }
    }
  }

  static unsigned long lastScanMs = 0;
  char uid[24];
  if (millis() - lastScanMs > 300) {
    lastScanMs = millis();
    if (rfid.pollUID(uid, sizeof(uid))) {
      Serial.print("[RFID] UID=");
      Serial.println(uid);

      bool inside = false;
      (void)getInsideByUid_(uid, inside);
      const bool nextInside = !inside;
      const char* dir = nextInside ? "IN" : "OUT";
      const long epoch = nowEpoch_();

      pendingAdd_(uid, dir, nextInside, epoch);

      lcd.printLine(2, String("UID: ") + uid);
      lcd.printLine(3, "Checking...");
      uart.send("CMD:RFID_CHECK");
      // No dedicated "checking" audio in your provided list.

      GoogleLogEvent ev;
      memset(&ev, 0, sizeof(ev));
      strncpy(ev.uid, uid, sizeof(ev.uid) - 1);
      strncpy(ev.status, dir, sizeof(ev.status) - 1);
      ev.epoch = epoch;
      (void)gsheet.enqueueEvent(ev);
    }
  }

  GoogleLogResult res;
  while (gsheet.tryDequeueResult(res)) {
    if (res.httpCode == 200 && res.ok) {
      PendingScan p;
      const bool hasP = pendingTake_(res.uid, p);
      const char* dir = hasP ? p.direction : "IN";
      const long epoch = hasP ? p.epoch : 0;

      // Always open the door on authorized RFID (IN/OUT is for logging only).
      // Use gateway command handler so door open voice is consistent.
      const unsigned long nowMs = millis();
      const bool doorTelemetryRecent = (shadow.lastRxMs != 0) && (nowMs - shadow.lastRxMs < 5000UL);
      const bool doorAlreadyOpen = doorTelemetryRecent && shadow.doorOpen;

      if (doorAlreadyOpen) {
        // Refresh Mega auto-close timer but do not spam voice.
        uart.send("CMD:DOOR_OPEN");
        uart.send("CMD:BEEP");
      } else {
        String err;
        (void)gatewayHandleCommand("DOOR_OPEN", err, "RFID");
      }

      // (Audio is handled above via gatewayHandleCommand or doorAlreadyOpen BEEP.)

      if (hasP) setInsideByUid_(res.uid, p.nextInside);

      const char* name = (res.name[0] != 0) ? res.name : "";
      (void)gatewayRfidAddCard(String(res.uid), String(name));
      gatewayRfidRecordScan(res.uid, true, name, dir, epoch);

      lcd.printLine(3, String("OK ") + dir);
      // Success audio handled above (door open/close).
      uart.send(String("CMD:HELLO:") + (name[0] != 0 ? name : "Ban"));
      continue;
    }

    if (res.httpCode == 200 && !res.ok) {
      PendingScan p;
      const bool hasP = pendingTake_(res.uid, p);
      gatewayRfidRecordScan(res.uid, false, "", "", hasP ? p.epoch : 0);

      lcd.printLine(3, "DENIED");
      // Play "wrong card" voice carefully: avoid spamming if the same card is held near the reader.
      {
        static char s_lastDeniedUid[24] = {0};
        static unsigned long s_lastDeniedAudioMs = 0;
        const unsigned long nowMs = millis();
        const bool sameUid = (s_lastDeniedUid[0] != 0) && (strncmp(s_lastDeniedUid, res.uid, sizeof(s_lastDeniedUid) - 1) == 0);
        const unsigned long cooldownMs = sameUid ? 6000UL : 2000UL;
        if ((nowMs - s_lastDeniedAudioMs) > cooldownMs) {
          if (audioReady()) audioPlay(audio_tracks::SAITHE);
          strncpy(s_lastDeniedUid, res.uid, sizeof(s_lastDeniedUid) - 1);
          s_lastDeniedUid[sizeof(s_lastDeniedUid) - 1] = 0;
          s_lastDeniedAudioMs = nowMs;
        }
      }
      uart.send("CMD:RFID_FAIL");

      // If user scans a wrong card while the door is open, close immediately.
      {
        const unsigned long nowMs = millis();
        const bool doorTelemetryRecent = (shadow.lastRxMs != 0) && (nowMs - shadow.lastRxMs < 5000UL);
        const bool doorAlreadyOpen = doorTelemetryRecent && shadow.doorOpen;
        if (doorAlreadyOpen) {
          String err;
          (void)gatewayHandleCommand("DOOR_CLOSE", err, "RFID_DENIED");
        }
      }
      continue;
    }

    if (res.httpCode == -10) {
      lcd.printLine(3, "Err: No WiFi");
      // Keep silent (no matching audio provided)
      uart.send("CMD:RFID_FAIL");
      continue;
    }

    String msg = (res.message[0] != 0) ? String(res.message) : String("fail");
    if (msg.length() > 20) msg = msg.substring(0, 20);
    lcd.printLine(3, String("Err ") + res.httpCode + ":" + msg);
    // Keep silent (no matching audio provided)
    uart.send("CMD:RFID_FAIL");
  }

  static unsigned long lastUiMs = 0;
  if (millis() - lastUiMs > 250UL) {
    lastUiMs = millis();
    renderDashboard();
  }

  gatewayStateLoop();
  delay(1);
}

