#include "system_monitor.h"
#include <Arduino.h>

void logSystem() {
  static unsigned long last = 0;
  if (millis() - last > 5000) {
    Serial.printf("Heap: %d\n", ESP.getFreeHeap());
    last = millis();
  }
}
