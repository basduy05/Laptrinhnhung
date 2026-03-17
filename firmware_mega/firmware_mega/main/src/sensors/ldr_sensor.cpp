#include "ldr_sensor.h"

void LdrSensor::begin(uint8_t analogPin, int threshold, bool darkWhenHigh) {
  pin = analogPin;
  thresholdValue = threshold;
  this->darkWhenHigh = darkWhenHigh;
  lastValue = analogRead(pin);
}

void LdrSensor::update() {
  lastValue = analogRead(pin);
}

int LdrSensor::value() const {
  return lastValue;
}

bool LdrSensor::isDark() const {
  if (darkWhenHigh) return lastValue >= thresholdValue;
  return lastValue <= thresholdValue;
}
