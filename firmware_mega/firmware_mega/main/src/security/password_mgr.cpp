#include "password_mgr.h"

void PasswordMgr::setPassword(const char* pwd) {
  if (!pwd) return;
  strncpy(password, pwd, sizeof(password) - 1);
  password[sizeof(password) - 1] = '\0';
}

bool PasswordMgr::verify(const char* input) const {
  if (!input) return false;
  return strcmp(input, password) == 0;
}
