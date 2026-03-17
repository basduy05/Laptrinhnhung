#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <Arduino.h>

class Ultrasonic {
public:
  void begin(uint8_t trig, uint8_t echo, unsigned long timeoutUs = 30000UL);
  void update();
  int distanceCm() const;
  unsigned long durationUs() const;
  bool ok() const;

private:
  uint8_t trigPin, echoPin;
  int distance = 999;
  unsigned long lastRead = 0;
  unsigned long timeoutUs = 30000UL;
  unsigned long lastDurationUs = 0;
  bool lastOk = false;
};

#endif
