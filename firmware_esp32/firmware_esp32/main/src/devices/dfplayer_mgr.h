#pragma once

#include <Arduino.h>
#include "DFRobotDFPlayerMini.h"

class DFPlayerMgr {
public:
  void begin();
  bool ready() const { return ready_; }

  void setVolume(uint8_t vol);
  void play(uint16_t track);

private:
  bool ready_ = false;
  uint8_t volume_ = 25;
  HardwareSerial mp3Serial_{1};
  DFRobotDFPlayerMini player_;
};
