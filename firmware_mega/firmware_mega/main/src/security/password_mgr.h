#ifndef PASSWORD_MGR_H
#define PASSWORD_MGR_H

#include <Arduino.h>
#include <string.h>

class PasswordMgr {
public:
  void setPassword(const char* pwd);
  bool verify(const char* input) const;

private:
  char password[16] = "666";
};

#endif
