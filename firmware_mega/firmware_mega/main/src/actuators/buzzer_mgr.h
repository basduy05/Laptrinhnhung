#ifndef BUZZER_MGR_H
#define BUZZER_MGR_H

#include <Arduino.h>

class BuzzerMgr {
public:
  void begin(uint8_t pin, bool activeLow = true, bool useTone = false, unsigned int toneHz = 2000);
  void alarm();
  void beep(unsigned int durationMs = 80);
  void off();
  void update();

private:
  enum Mode { MODE_OFF, MODE_ALARM, MODE_BEEP };

  uint8_t buzzerPin = 255;
  bool activeLow = true;
  bool useTone = false;
  unsigned int toneHz = 2000;
  Mode mode = MODE_OFF;
  unsigned long beepUntilMs = 0;

  void writeOn();
  void writeOff();
};

#endif
