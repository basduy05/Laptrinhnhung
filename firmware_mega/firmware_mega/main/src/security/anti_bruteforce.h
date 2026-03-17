#ifndef ANTI_BRUTEFORCE_H
#define ANTI_BRUTEFORCE_H

class AntiBruteforce {
public:
  void fail();
  bool locked() const;
  void reset();

private:
  int failCount = 0;
  unsigned long lockUntil = 0;
};

#endif
