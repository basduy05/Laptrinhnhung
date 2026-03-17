#ifndef GAS_SENSOR_H
#define GAS_SENSOR_H

#include <Arduino.h>

class GasSensor {
public:
  void begin(uint8_t pin, int threshold);
  void update();
  int value() const;
  bool isDanger() const;  // Now debounced

private:
  uint8_t analogPin;
  int threshold;
  int gasValue = 0;
  bool lastDangerValue = false;
  unsigned long dangerSinceMs = 0;
};

#endif