#include "emergency_button.h"

void EmergencyButton::begin(uint8_t pin, bool low) {
	buttonPin = pin;
	activeLow = low;
	pinMode(buttonPin, activeLow ? INPUT_PULLUP : INPUT);
	lastChangeMs = millis();
	lastRawPressed = false;
	isPressed = false;
	update();
}

void EmergencyButton::update() {
	const int v = digitalRead(buttonPin);
	const bool rawPressed = activeLow ? (v == LOW) : (v == HIGH);
	const unsigned long now = millis();

	if (rawPressed != lastRawPressed) {
		lastRawPressed = rawPressed;
		lastChangeMs = now;
	}

	if ((now - lastChangeMs) >= DEBOUNCE_MS) {
		isPressed = lastRawPressed;
	}
}

bool EmergencyButton::pressed() const {
	return isPressed;
}
