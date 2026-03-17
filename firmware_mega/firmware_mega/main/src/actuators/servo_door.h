#ifndef SERVO_DOOR_H
#define SERVO_DOOR_H

#include <Arduino.h>
#include <ServoEasing.h>

class ServoDoor {
public:
  void begin(uint8_t pin, int openAngle, int closeAngle);
  void open();
  void close();
  void update();
  bool isOpen() const;
  void enablePowerSaving(bool enable, unsigned long detachDelayMs = 2000UL);

private:
  ServoEasing servo;
  uint8_t servoPin;
  int angleOpen, angleClose;
  bool doorOpen = false;
  bool powerSavingEnabled = false;
  unsigned long lastMoveMs = 0;
  unsigned long detachDelayMs = 2000UL;
  bool isAttached = false;
};

#endif
