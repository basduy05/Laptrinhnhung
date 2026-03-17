#include "anti_bruteforce.h"
#include <Arduino.h>

void AntiBruteforce::fail() {
  failCount++;
  if (failCount >= 5) lockUntil = millis() + 30000;
}

bool AntiBruteforce::locked() const {
  return millis() < lockUntil;
}

void AntiBruteforce::reset() {
  failCount = 0;
  lockUntil = 0;
}
