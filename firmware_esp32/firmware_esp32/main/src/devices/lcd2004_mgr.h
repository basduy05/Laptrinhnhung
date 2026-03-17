#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#include "../core/config.h"

class LCD2004Mgr {
public:
  LCD2004Mgr(uint8_t i2cAddr = LCD_I2C_ADDR, uint8_t cols = 20, uint8_t rows = 4);
  void begin();
  void clear();
  void printLine(uint8_t row, const String& text);

private:
  LiquidCrystal_I2C lcd_;
  String last_[4];
};
