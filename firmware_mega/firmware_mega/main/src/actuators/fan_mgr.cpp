#include "fan_mgr.h"

void FanMgr::begin(uint8_t pin, bool isActiveLow) {
  pinRelay = pin;
  activeLow = isActiveLow;
  pinMode(pinRelay, OUTPUT);
  // Default OFF
  digitalWrite(pinRelay, activeLow ? HIGH : LOW);
}

void FanMgr::set(bool on) {
  digitalWrite(pinRelay, on ? (activeLow ? LOW : HIGH)
                            : (activeLow ? HIGH : LOW));
  running = on;
}

bool FanMgr::isRunning() const {
  return running;
}
