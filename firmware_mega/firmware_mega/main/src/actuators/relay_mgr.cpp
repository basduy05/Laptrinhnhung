#include "relay_mgr.h"

void RelayMgr::begin(uint8_t pin, bool low) {
  relayPin = pin;
  activeLow = low;
  pinMode(relayPin, OUTPUT);
  off();
}

void RelayMgr::on() {
  digitalWrite(relayPin, activeLow ? LOW : HIGH);
  isOn = true;
}

void RelayMgr::off() {
  digitalWrite(relayPin, activeLow ? HIGH : LOW);
  isOn = false;
}

bool RelayMgr::state() const {
  return isOn;
}
