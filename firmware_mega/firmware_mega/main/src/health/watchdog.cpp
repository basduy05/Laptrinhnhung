#include "watchdog.h"
#include <avr/wdt.h>

void Watchdog::begin() {
  wdt_enable(WDTO_4S);  // Increased from 2s to 4s for reliability
}

void Watchdog::feed() {
  wdt_reset();
}
