#ifndef LCD1604_MGR_H
#define LCD1604_MGR_H

#include <LiquidCrystal_I2C.h>
#include <string.h>

class LCD1604Mgr {
public:
  LCD1604Mgr(uint8_t addr);
  void begin();
  void showLocked(const char* buf);
  void showMenu(const char* item);
  void showMenuNav(const char* item);
  void showEmergency(const char* msg);
  void showStatus2(const char* line0, const char* line1);

private:
  LiquidCrystal_I2C lcd;
  char lastLine0[17] = {0};
  char lastLine1[17] = {0};
  bool hasCache = false;

  void setLines(const char* line0, const char* line1);
};

#endif
