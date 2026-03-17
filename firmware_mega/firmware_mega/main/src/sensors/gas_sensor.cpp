#include "gas_sensor.h"

void GasSensor::begin(uint8_t pin, int th) {
  analogPin = pin;
  threshold = th;
  lastDangerValue = false;
  dangerSinceMs = 0;
}

void GasSensor::update() {
  gasValue = analogRead(analogPin);
  
  // Debounce: require danger state stable for 500ms
  bool currentDanger = gasValue > threshold;
  if (currentDanger != lastDangerValue) {
    lastDangerValue = currentDanger;
    dangerSinceMs = millis();
  }
}

int GasSensor::value() const {
  return gasValue;
}

bool GasSensor::isDanger() const {
  // Only report danger if state stable for 500ms
  return lastDangerValue && (millis() - dangerSinceMs >= 500UL);
}
