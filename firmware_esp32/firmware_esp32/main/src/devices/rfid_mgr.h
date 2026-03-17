#pragma once

#include <Arduino.h>

class RFIDMgr {
public:
  void begin();
  bool pollUID(char* outUid, size_t outLen);

private:
  bool inited_ = false;
};
