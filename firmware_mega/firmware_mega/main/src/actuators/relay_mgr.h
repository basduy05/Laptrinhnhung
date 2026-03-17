#ifndef RELAY_MGR_H
#define RELAY_MGR_H

#include <Arduino.h>

class RelayMgr {
public:
  void begin(uint8_t pin, bool activeLow = true);
  void on();
  void off();
  bool state() const;

private:
  uint8_t relayPin;
  bool activeLow;
  bool isOn = false;
};

#endif
