#ifndef IR_SENSOR_H
#define IR_SENSOR_H

#include <Arduino.h>

class IRSensor {
public:
  void begin(uint8_t pin, uint8_t detectedLevel = LOW, bool usePullup = false);
  void update();
  bool motionDetected() const;
  uint8_t raw() const;

private:
  uint8_t pinIR;
  uint8_t detectedLevel = LOW;
  bool detected = false;
  uint8_t lastRaw = HIGH;
};

#endif
