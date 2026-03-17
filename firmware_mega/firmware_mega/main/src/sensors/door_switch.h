#ifndef DOOR_SWITCH_H
#define DOOR_SWITCH_H

#include <Arduino.h>

class DoorSwitch {
public:
  void begin(uint8_t pin);
  bool isOpen() const;

private:
  uint8_t pinDoor;
};

#endif
