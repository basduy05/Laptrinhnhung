#pragma once

#include <stdint.h>

bool connectWiFi(uint32_t timeoutMs = 15000);

// Periodically call in loop() to keep WiFi connected.
void wifiLoop();

bool wifiConnected();
