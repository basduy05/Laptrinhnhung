#include "power_monitor.h"

void PowerMonitor::begin(uint8_t pin) {
  sensePin = pin;
  pinMode(sensePin, INPUT);
}

bool PowerMonitor::isMainPowerLost() const {
  return digitalRead(sensePin) == LOW;
}
