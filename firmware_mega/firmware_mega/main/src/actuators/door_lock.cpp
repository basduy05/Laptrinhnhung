#include "door_lock.h"

void DoorLock::begin(uint8_t pin, bool low) {
  lockPin = pin;
  activeLow = low;
  pinMode(lockPin, OUTPUT);
  lock();
}

void DoorLock::lock() {
  digitalWrite(lockPin, activeLow ? HIGH : LOW);
  locked = true;
}

void DoorLock::unlock() {
  digitalWrite(lockPin, activeLow ? LOW : HIGH);
  locked = false;
}

bool DoorLock::isLocked() const {
  return locked;
}
