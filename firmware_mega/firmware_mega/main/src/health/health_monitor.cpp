#include "health_monitor.h"
#include <Arduino.h>

extern int __heap_start, *__brkval;

void HealthMonitor::begin() {
  startTime = millis();
}

void HealthMonitor::update() {}

unsigned long HealthMonitor::uptime() const {
  return (millis() - startTime) / 1000;
}

int HealthMonitor::getFreeRAM() const {
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
