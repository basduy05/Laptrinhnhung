#ifndef TIMER_UTIL_H
#define TIMER_UTIL_H

#include <Arduino.h>

class SimpleTimer {
public:
  bool expired(unsigned long interval) {
    if (millis() - last >= interval) {
      last = millis();
      return true;
    }
    return false;
  }
private:
  unsigned long last = 0;
};

#endif
