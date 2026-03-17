#include "door_switch.h"
#include "../core/config.h"

void DoorSwitch::begin(uint8_t pin) {
  pinDoor = pin;
  pinMode(pinDoor, INPUT_PULLUP);
}

bool DoorSwitch::isOpen() const {
  return digitalRead(pinDoor) == DOOR_SWITCH_OPEN_LEVEL;
}
