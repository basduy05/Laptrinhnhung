#ifndef FIRE_SENSOR_H
#define FIRE_SENSOR_H

#include <Arduino.h>

class FireSensor {
public:
  void begin(uint8_t pin);
  void update();
  bool detected() const;

private:
  uint8_t digitalPin;
  bool lastFireValue = false;
  unsigned long fireSinceMs = 0;
};

#endif
