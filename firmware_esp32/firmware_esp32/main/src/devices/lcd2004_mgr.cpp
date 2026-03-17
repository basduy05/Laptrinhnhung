#include "lcd2004_mgr.h"

#include <Wire.h>

#include "../core/config.h"

LCD2004Mgr::LCD2004Mgr(uint8_t i2cAddr, uint8_t cols, uint8_t rows) : lcd_(i2cAddr, cols, rows) {}

void LCD2004Mgr::begin() {
  Wire.begin();
  lcd_.init();
  lcd_.backlight();

  for (int i = 0; i < 4; i++) {
    last_[i] = "";
  }
}

void LCD2004Mgr::clear() {
  lcd_.clear();
  for (int i = 0; i < 4; i++) {
    last_[i] = "";
  }
}

void LCD2004Mgr::printLine(uint8_t row, const String& text) {
  if (row > 3) return;

  String t = text;
  if (t.length() > 20) {
    t = t.substring(0, 20);
  } else if (t.length() < 20) {
    while (t.length() < 20) {
      t += ' ';
    }
  }

  if (last_[row] == t) return;
  last_[row] = t;

  lcd_.setCursor(0, row);
  lcd_.print(t);
}
