#include "api_router.h"

#include <ArduinoJson.h>

#include "../comm/uart_link.h"
#include "../gateway/gateway.h"

extern UARTLink uart;

static void sendJson(WebServer& server, int code, const String& json) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(code, "application/json", json);
}

static void sendText(WebServer& server, int code, const String& text) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(code, "text/plain", text);
}

void registerAPI(WebServer& server) {

  server.on("/", HTTP_GET, [&](){
    sendText(server, 200, "MIKI HUB ONLINE");
  });

  // CORS preflight
  server.on("/sensors", HTTP_OPTIONS, [&](){ sendText(server, 204, ""); });
  server.on("/control", HTTP_OPTIONS, [&](){ sendText(server, 204, ""); });
  server.on("/rfid/last", HTTP_OPTIONS, [&](){ sendText(server, 204, ""); });
  server.on("/rfid/cards", HTTP_OPTIONS, [&](){ sendText(server, 204, ""); });
  server.on("/rfid/history", HTTP_OPTIONS, [&](){ sendText(server, 204, ""); });
  server.on("/caps", HTTP_OPTIONS, [&](){ sendText(server, 204, ""); });
  server.on("/pin/list", HTTP_OPTIONS, [&](){ sendText(server, 204, ""); });
  server.on("/pin/emergency", HTTP_OPTIONS, [&](){ sendText(server, 204, ""); });

  // Web dashboard compatibility
  server.on("/sensors", HTTP_GET, [&](){
    sendJson(server, 200, gatewayBuildSensorsJson(false));
  });

  // /control?cmd=FAN1_ON
  server.on("/control", HTTP_GET, [&](){
    if (!server.hasArg("cmd")) {
      StaticJsonDocument<128> doc;
      doc["ok"] = false;
      doc["error"] = "missing_cmd";
      String out; serializeJson(doc, out);
      sendJson(server, 400, out);
      return;
    }

    const String cmd = server.arg("cmd");
    String err;
    const bool ok = gatewayHandleCommand(cmd, err, "WEB");

    StaticJsonDocument<256> doc;
    doc["ok"] = ok;
    doc["cmd"] = cmd;
    doc["ts_ms"] = (unsigned long)millis();
    if (!ok) doc["error"] = err;

    // Immediate state snapshot for UI to sync right away
    StaticJsonDocument<1536> stateDoc;
    DeserializationError se = deserializeJson(stateDoc, gatewayBuildSensorsJson(true));
    if (!se) {
      doc["state"] = stateDoc.as<JsonObject>();
    }
    String out; serializeJson(doc, out);
    sendJson(server, ok ? 200 : 400, out);
  });

  server.on("/caps", HTTP_GET, [&](){
    sendJson(server, 200, gatewayBuildCapsJson());
  });

  // RFID endpoints
  server.on("/rfid/last", HTTP_GET, [&](){
    sendJson(server, 200, gatewayBuildRfidLastJson());
  });

  server.on("/rfid/cards", HTTP_GET, [&](){
    // Compatibility: allow HTML forms using query args
    // Example: /rfid/cards?action=add&uid=...&name=...
    if (server.hasArg("action")) {
      const String action = server.arg("action");
      const String uid = server.hasArg("uid") ? server.arg("uid")
                        : (server.hasArg("card") ? server.arg("card")
                        : (server.hasArg("rfid") ? server.arg("rfid") : String("")));
      const String name = server.hasArg("name") ? server.arg("name")
                         : (server.hasArg("card_name") ? server.arg("card_name") : String(""));
      bool ok = false;
      if (action == "add") ok = gatewayRfidAddCard(uid, name);
      else if (action == "del") ok = gatewayRfidDelCard(uid);

      StaticJsonDocument<128> resp;
      resp["ok"] = ok;
      resp["action"] = action;
      resp["uid"] = uid;
      if (action == "add") resp["name"] = name;
      String out; serializeJson(resp, out);
      sendJson(server, ok ? 200 : 400, out);
      return;
    }
    sendJson(server, 200, gatewayBuildRfidCardsJson());
  });

  // POST /rfid/cards  body: {"action":"add","uid":"A1B2C3D4","name":"Nguyen Van A"}
  server.on("/rfid/cards", HTTP_POST, [&](){
    String action;
    String uid;
    String name;

    const String body = server.arg("plain");
    if (body.length() > 0) {
      StaticJsonDocument<192> doc;
      DeserializationError e = deserializeJson(doc, body);
      if (!e) {
        action = String((const char*)doc["action"]);
        uid = String((const char*)doc["uid"]);
        name = String((const char*)doc["name"]);
      }
    }

    // Fallback for application/x-www-form-urlencoded
    if (action.length() == 0 && server.hasArg("action")) {
      action = server.arg("action");
      uid = server.hasArg("uid") ? server.arg("uid")
          : (server.hasArg("card") ? server.arg("card")
          : (server.hasArg("rfid") ? server.arg("rfid") : String("")));
      name = server.hasArg("name") ? server.arg("name")
           : (server.hasArg("card_name") ? server.arg("card_name") : String(""));
    }

    if (action.length() == 0) {
      StaticJsonDocument<128> resp;
      resp["ok"] = false;
      resp["error"] = "missing_action";
      String out; serializeJson(resp, out);
      sendJson(server, 400, out);
      return;
    }

    bool ok = false;
    if (action == "add") ok = gatewayRfidAddCard(uid, name);
    else if (action == "del") ok = gatewayRfidDelCard(uid);

    StaticJsonDocument<128> resp;
    resp["ok"] = ok;
    resp["action"] = action;
    resp["uid"] = uid;
    if (action == "add") resp["name"] = name;
    String out; serializeJson(resp, out);
    sendJson(server, ok ? 200 : 400, out);
  });

  server.on("/rfid/history", HTTP_GET, [&](){
    sendJson(server, 200, gatewayBuildRfidHistoryJson());
  });

  // PIN/password endpoints
  server.on("/pin/list", HTTP_GET, [&](){
    // Compatibility: allow ?action=add|del&pin=...
    if (server.hasArg("action")) {
      const String action = server.arg("action");
      const String pin = server.hasArg("pin") ? server.arg("pin")
                        : (server.hasArg("password") ? server.arg("password")
                        : (server.hasArg("pass") ? server.arg("pass") : String("")));
      bool ok = false;
      if (action == "add") ok = gatewayPinAdd(pin);
      else if (action == "del") ok = gatewayPinDel(pin);

      StaticJsonDocument<128> resp;
      resp["ok"] = ok;
      resp["action"] = action;
      resp["pin"] = "****";
      String out; serializeJson(resp, out);
      sendJson(server, ok ? 200 : 400, out);
      return;
    }
    sendJson(server, 200, gatewayBuildPinsJson());
  });

  // Emergency PIN endpoints
  server.on("/pin/emergency", HTTP_GET, [&](){
    // Compatibility: allow ?action=add|del&pin=...
    if (server.hasArg("action")) {
      const String action = server.arg("action");
      const String pin = server.hasArg("pin") ? server.arg("pin")
                        : (server.hasArg("password") ? server.arg("password")
                        : (server.hasArg("pass") ? server.arg("pass") : String("")));
      bool ok = false;
      if (action == "add") ok = gatewayEmergencyPinAdd(pin);
      else if (action == "del") ok = gatewayEmergencyPinDel(pin);

      StaticJsonDocument<128> resp;
      resp["ok"] = ok;
      resp["action"] = action;
      resp["pin"] = "****";
      String out; serializeJson(resp, out);
      sendJson(server, ok ? 200 : 400, out);
      return;
    }
    sendJson(server, 200, gatewayBuildEmergencyPinsJson());
  });

  // POST /pin/emergency  body: {"action":"add","pin":"1234"}
  server.on("/pin/emergency", HTTP_POST, [&](){
    String action;
    String pin;

    const String body = server.arg("plain");
    if (body.length() > 0) {
      StaticJsonDocument<192> doc;
      DeserializationError e = deserializeJson(doc, body);
      if (!e) {
        action = String((const char*)doc["action"]);
        pin = String((const char*)doc["pin"]);
      }
    }

    if (action.length() == 0 && server.hasArg("action")) {
      action = server.arg("action");
      pin = server.hasArg("pin") ? server.arg("pin")
          : (server.hasArg("password") ? server.arg("password")
          : (server.hasArg("pass") ? server.arg("pass") : String("")));
    }

    if (action.length() == 0) {
      StaticJsonDocument<128> resp;
      resp["ok"] = false;
      resp["error"] = "missing_action";
      String out; serializeJson(resp, out);
      sendJson(server, 400, out);
      return;
    }

    bool ok = false;
    if (action == "add") ok = gatewayEmergencyPinAdd(pin);
    else if (action == "del") ok = gatewayEmergencyPinDel(pin);

    StaticJsonDocument<128> resp;
    resp["ok"] = ok;
    resp["action"] = action;
    resp["pin"] = "****";
    String out; serializeJson(resp, out);
    sendJson(server, ok ? 200 : 400, out);
  });

  // POST /pin/list  body: {"action":"add","pin":"1234"}
  server.on("/pin/list", HTTP_POST, [&](){
    String action;
    String pin;

    const String body = server.arg("plain");
    if (body.length() > 0) {
      StaticJsonDocument<192> doc;
      DeserializationError e = deserializeJson(doc, body);
      if (!e) {
        action = String((const char*)doc["action"]);
        pin = String((const char*)doc["pin"]);
      }
    }

    if (action.length() == 0 && server.hasArg("action")) {
      action = server.arg("action");
      pin = server.hasArg("pin") ? server.arg("pin")
          : (server.hasArg("password") ? server.arg("password")
          : (server.hasArg("pass") ? server.arg("pass") : String("")));
    }

    if (action.length() == 0) {
      StaticJsonDocument<128> resp;
      resp["ok"] = false;
      resp["error"] = "missing_action";
      String out; serializeJson(resp, out);
      sendJson(server, 400, out);
      return;
    }

    bool ok = false;
    if (action == "add") ok = gatewayPinAdd(pin);
    else if (action == "del") ok = gatewayPinDel(pin);

    StaticJsonDocument<128> resp;
    resp["ok"] = ok;
    resp["action"] = action;
    resp["pin"] = "****";
    String out; serializeJson(resp, out);
    sendJson(server, ok ? 200 : 400, out);
  });

  // Fan 1
  server.on("/api/fan1/on", HTTP_GET, [&](){
    String err;
    (void)gatewayHandleCommand("FAN1_ON", err, "WEB");
    sendText(server, 200, "OK");
  });
  server.on("/api/fan1/off", HTTP_GET, [&](){
    String err;
    (void)gatewayHandleCommand("FAN1_OFF", err, "WEB");
    sendText(server, 200, "OK");
  });

  // Fan 2
  server.on("/api/fan2/on", HTTP_GET, [&](){
    String err;
    (void)gatewayHandleCommand("FAN2_ON", err, "WEB");
    sendText(server, 200, "OK");
  });
  server.on("/api/fan2/off", HTTP_GET, [&](){
    String err;
    (void)gatewayHandleCommand("FAN2_OFF", err, "WEB");
    sendText(server, 200, "OK");
  });

  // Door + Lock
  server.on("/api/door/open", HTTP_GET, [&](){
    String err;
    (void)gatewayHandleCommand("DOOR_OPEN", err, "WEB");
    sendText(server, 200, "OK");
  });
  server.on("/api/door/close", HTTP_GET, [&](){
    String err;
    (void)gatewayHandleCommand("DOOR_CLOSE", err, "WEB");
    sendText(server, 200, "OK");
  });
  server.on("/api/lock", HTTP_GET, [&](){
    uart.send("CMD:LOCK");
    sendText(server, 200, "OK");
  });
  server.on("/api/unlock", HTTP_GET, [&](){
    uart.send("CMD:UNLOCK");
    sendText(server, 200, "OK");
  });

  // Curtain
  server.on("/api/curtain/open", HTTP_GET, [&](){
    String err;
    (void)gatewayHandleCommand("CURTAIN_OPEN", err, "WEB");
    sendText(server, 200, "OK");
  });
  server.on("/api/curtain/close", HTTP_GET, [&](){
    String err;
    (void)gatewayHandleCommand("CURTAIN_CLOSE", err, "WEB");
    sendText(server, 200, "OK");
  });
  server.on("/api/curtain/stop", HTTP_GET, [&](){
    String err;
    (void)gatewayHandleCommand("CURTAIN_STOP", err, "WEB");
    sendText(server, 200, "OK");
  });

  // LED presets supported by Mega protocol
  server.on("/api/led/off", HTTP_GET, [&](){
    String err;
    (void)gatewayHandleCommand("LIGHTS_ALL_OFF", err, "WEB");
    sendText(server, 200, "OK");
  });
  server.on("/api/led/rainbow", HTTP_GET, [&](){
    String err;
    (void)gatewayHandleCommand("LIGHTS_ALL_COLOR_RAINBOW", err, "WEB");
    sendText(server, 200, "OK");
  });
  server.on("/api/led/all", HTTP_GET, [&](){
    String err;
    (void)gatewayHandleCommand("LIGHTS_ALL_ON", err, "WEB");
    sendText(server, 200, "OK");
  });

}
