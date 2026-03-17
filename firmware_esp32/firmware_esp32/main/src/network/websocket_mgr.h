#pragma once

#include <Arduino.h>

// Async websocket is disabled to avoid lwIP core-lock PANIC on some ESP32 core/library combos.
// Keep stubs so the project compiles if older code still includes this header.
inline void wsBroadcast(const String&) {}
