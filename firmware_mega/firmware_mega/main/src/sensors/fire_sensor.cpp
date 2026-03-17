#include "fire_sensor.h"

void FireSensor::begin(uint8_t pin) {
  digitalPin = pin;
  pinMode(digitalPin, INPUT);
  lastFireValue = false;
  fireSinceMs = 0;
}

void FireSensor::update() {
  bool currentFire = (digitalRead(digitalPin) == LOW);
  
  // Debounce: require fire state stable for 500ms
  if (currentFire != lastFireValue) {
    lastFireValue = currentFire;
    fireSinceMs = millis();
  }
}

bool FireSensor::detected() const {
  // Only report fire if state stable for 500ms
  return lastFireValue && (millis() - fireSinceMs >= 500UL);
}
