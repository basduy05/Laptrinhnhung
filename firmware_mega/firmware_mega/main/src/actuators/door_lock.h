#ifndef DOOR_LOCK_H
#define DOOR_LOCK_H

#include <Arduino.h>

class DoorLock {
public:
  void begin(uint8_t pin, bool activeLow = true);
  void lock();
  void unlock();
  bool isLocked() const;

private:
  uint8_t lockPin;
  bool activeLow;
  bool locked = true;
};

#endif
