#ifndef AUTH_SESSION_H
#define AUTH_SESSION_H

class AuthSession {
public:
  void login();
  void logout();
  bool isLoggedIn() const;

private:
  bool logged = false;
};

#endif
