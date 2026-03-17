#include "auth_session.h"

void AuthSession::login() {
  logged = true;
}

void AuthSession::logout() {
  logged = false;
}

bool AuthSession::isLoggedIn() const {
  return logged;
}
