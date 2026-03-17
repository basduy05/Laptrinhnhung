#include <Arduino.h>
#include <ServoEasing.hpp>

#include "servo_door.h"

#include "../core/config.h"

namespace {

static void servoPower_(bool on) {
  pinMode(PIN_RELAY_SERVO_PWR, OUTPUT);
  if (SERVO_RELAY_ACTIVE_LOW) {
    digitalWrite(PIN_RELAY_SERVO_PWR, on ? LOW : HIGH);
  } else {
    digitalWrite(PIN_RELAY_SERVO_PWR, on ? HIGH : LOW);
  }
}

static void servoPowerOnAndSettle_() {
  // Give the servo supply a moment to stabilize before attach/move.
  servoPower_(true);
  delay(200);
}

} // namespace

void ServoDoor::begin(uint8_t pin, int openA, int closeA) {
  servoPin = pin;
  angleOpen = openA;
  angleClose = closeA;
  servoPowerOnAndSettle_();
  servo.attach(servoPin);
  servo.write(angleClose);
  isAttached = true;
  lastMoveMs = millis();
}

void ServoDoor::enablePowerSaving(bool enable, unsigned long delayMs) {
  powerSavingEnabled = enable;
  detachDelayMs = delayMs;
}

void ServoDoor::open() {
  if (!isAttached) {
    servoPowerOnAndSettle_();
    servo.attach(servoPin);
    isAttached = true;
  }
  // Use non-blocking write instead of blocking easeTo()
  servo.write(angleOpen);
  doorOpen = true;
  lastMoveMs = millis();
}

void ServoDoor::close() {
  if (!isAttached) {
    servoPowerOnAndSettle_();
    servo.attach(servoPin);
    isAttached = true;
  }
  // Use non-blocking write instead of blocking easeTo()
  servo.write(angleClose);
  doorOpen = false;
  lastMoveMs = millis();
}

void ServoDoor::update() {
  // Auto-detach servo after idle to save power
  if (powerSavingEnabled && isAttached) {
    if ((millis() - lastMoveMs) > detachDelayMs) {
      servo.detach();
      isAttached = false;
      servoPower_(false);
    }
  }
}

bool ServoDoor::isOpen() const {
  return doorOpen;
}
