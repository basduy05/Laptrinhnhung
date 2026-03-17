#include "lcd1604_mgr.h"
#include <stdio.h>

static void format16(char out[17], const char* in) {
  for (uint8_t i = 0; i < 16; i++) out[i] = ' ';
  out[16] = '\0';
  if (!in) return;
  for (uint8_t i = 0; i < 16 && in[i] != '\0'; i++) out[i] = in[i];
}

LCD1604Mgr::LCD1604Mgr(uint8_t addr)
  : lcd(addr, 16, 2) {}

void LCD1604Mgr::begin() {
  lcd.init();
  lcd.backlight();
  hasCache = false;
  lastLine0[0] = 0;
  lastLine1[0] = 0;
  lcd.clear();
}

void LCD1604Mgr::setLines(const char* line0, const char* line1) {
  char buf0[17];
  char buf1[17];
  format16(buf0, line0);
  format16(buf1, line1);

  if (!hasCache || strncmp(buf0, lastLine0, 16) != 0) {
    memcpy(lastLine0, buf0, sizeof(lastLine0));
    lcd.setCursor(0, 0);
    lcd.print(buf0);
  }

  if (!hasCache || strncmp(buf1, lastLine1, 16) != 0) {
    memcpy(lastLine1, buf1, sizeof(lastLine1));
    lcd.setCursor(0, 1);
    lcd.print(buf1);
  }

  hasCache = true;
}

void LCD1604Mgr::showLocked(const char* buf) {
  setLines("Nhap MK:", buf);
}

void LCD1604Mgr::showMenu(const char* item) {
  char line0[17];
  snprintf(line0, sizeof(line0), ">%s", item ? item : "");
  setLines(line0, "");
}

void LCD1604Mgr::showMenuNav(const char* item) {
  char line0[17];
  snprintf(line0, sizeof(line0), "> %s", item ? item : "");
  setLines(line0, "2:Xuong 8:Len");
}

void LCD1604Mgr::showEmergency(const char* msg) {
  setLines("!! NGUY HIEM !!", msg);
}

void LCD1604Mgr::showStatus2(const char* line0, const char* line1) {
  setLines(line0, line1);
}
