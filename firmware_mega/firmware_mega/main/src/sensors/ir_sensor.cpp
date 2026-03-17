#include "ir_sensor.h"

void IRSensor::begin(uint8_t pin, uint8_t detectedLevel, bool usePullup) {
  pinIR = pin;
  this->detectedLevel = detectedLevel;
  pinMode(pinIR, usePullup ? INPUT_PULLUP : INPUT);
}

void IRSensor::update() {
  lastRaw = digitalRead(pinIR);
  detected = (lastRaw == detectedLevel);
}

bool IRSensor::motionDetected() const {
  return detected;
}

uint8_t IRSensor::raw() const {
  return lastRaw;
}
