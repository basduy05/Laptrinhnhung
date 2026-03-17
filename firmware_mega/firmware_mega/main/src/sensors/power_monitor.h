#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include <Arduino.h>

class PowerMonitor {
public:
  void begin(uint8_t pin);
  bool isMainPowerLost() const;

private:
  uint8_t sensePin;
};

#endif
