#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <Arduino.h>

class Heartbeat {
public:
  void begin(unsigned long intervalMs);
  void update();
  bool isAlive() const;

private:
  unsigned long interval;
  unsigned long lastBeat = 0;
  bool alive = false;
};

#endif
