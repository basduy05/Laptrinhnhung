#pragma once
#include <Arduino.h>

struct ParsedMsg {
  String key;
  String value;
};

bool parsePacket(const String& raw, ParsedMsg& out);
