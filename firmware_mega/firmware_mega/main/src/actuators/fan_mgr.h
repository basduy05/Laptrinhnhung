#ifndef FAN_MGR_H
#define FAN_MGR_H

#include <Arduino.h>

class FanMgr {
public:
  void begin(uint8_t pin, bool activeLow = true);
  void set(bool on);
  bool isRunning() const;
  bool isOn() const { return running; }

private:
  uint8_t pinRelay;
  bool activeLow = true;
  bool running = false;
};

#endif
