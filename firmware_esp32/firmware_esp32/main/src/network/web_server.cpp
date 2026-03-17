#include "web_server.h"
#include <WebServer.h>
#include "api_router.h"
#include "wifi_mgr.h"

static WebServer server(80);

void startWeb() {
  (void)connectWiFi();
  registerAPI(server);
  server.begin();
}

void webLoop() {
  wifiLoop();
  server.handleClient();
}
