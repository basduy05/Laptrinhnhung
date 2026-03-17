#include "heartbeat.h"

void Heartbeat::begin(unsigned long intervalMs) {
  interval = intervalMs;
  lastBeat = millis();
}

void Heartbeat::update() {
  if (millis() - lastBeat >= interval) {
    alive = !alive;
    lastBeat = millis();
  }
}

bool Heartbeat::isAlive() const {
  return alive;
}
