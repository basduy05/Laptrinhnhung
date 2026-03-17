#include "buzzer_mgr.h"

void BuzzerMgr::begin(uint8_t pin, bool activeLow, bool useTone, unsigned int toneHz) {
  buzzerPin = pin;
  this->activeLow = activeLow;
  this->useTone = useTone;
  this->toneHz = toneHz;
  pinMode(buzzerPin, OUTPUT);
  off();
}

void BuzzerMgr::alarm() {
  mode = MODE_ALARM;
}

void BuzzerMgr::beep(unsigned int durationMs) {
  if (mode == MODE_ALARM) return;
  mode = MODE_BEEP;
  beepUntilMs = millis() + (unsigned long)durationMs;
  writeOn();
}

void BuzzerMgr::off() {
  mode = MODE_OFF;
  beepUntilMs = 0;
  writeOff();
}

void BuzzerMgr::update() {
  if (mode == MODE_OFF) return;

  const unsigned long now = millis();

  if (mode == MODE_BEEP) {
    if ((long)(now - beepUntilMs) >= 0) {
      off();
    } else {
      writeOn();
    }
    return;
  }

  // MODE_ALARM: 200ms toggle
  if (((now / 200UL) % 2UL) == 0) writeOn();
  else writeOff();
}

void BuzzerMgr::writeOn() {
  if (useTone && !activeLow) {
    tone(buzzerPin, toneHz);
    return;
  }
  digitalWrite(buzzerPin, activeLow ? LOW : HIGH);
}

void BuzzerMgr::writeOff() {
  if (useTone && !activeLow) {
    noTone(buzzerPin);
    return;
  }
  digitalWrite(buzzerPin, activeLow ? HIGH : LOW);
}
