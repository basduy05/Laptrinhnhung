#ifndef KEYPAD_MGR_H
#define KEYPAD_MGR_H

#include <Arduino.h>

void keypadInit();
void keypadUpdate();

// Returns 0 if no key pressed
char keypadReadKey();

#endif
