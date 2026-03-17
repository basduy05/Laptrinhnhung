#ifndef EMERGENCY_BUTTON_H
#define EMERGENCY_BUTTON_H

#include <Arduino.h>

class EmergencyButton {
public:
	void begin(uint8_t pin, bool activeLow = true);
	void update();
	bool pressed() const;

private:
	uint8_t buttonPin = 0;
	bool activeLow = true;
	bool isPressed = false;
	bool lastRawPressed = false;
	unsigned long lastChangeMs = 0;
	static const unsigned long DEBOUNCE_MS = 50;
};

#endif
