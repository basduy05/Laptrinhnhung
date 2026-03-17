#ifndef LDR_SENSOR_H
#define LDR_SENSOR_H

#include <Arduino.h>

class LdrSensor {
public:
  void begin(uint8_t analogPin, int threshold, bool darkWhenHigh = true);
  void update();

  int value() const;
  bool isDark() const;

private:
  uint8_t pin = A0;
  int thresholdValue = 600;
  int lastValue = 0;
  bool darkWhenHigh = true;
};

#endif
