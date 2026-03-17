#include "ultrasonic.h"

static const unsigned long MIN_INTERVAL_MS = 80;      // faster updates for responsiveness
static const unsigned long MAX_PULSE_TIMEOUT_US = 30000UL; // cap to keep loop responsive
static const unsigned long VALID_HOLD_MS = 450;       // hold last valid reading briefly to avoid dropouts
static const int MIN_VALID_CM = 2;                    // HC-SR04 unreliable below ~2cm
static const int MAX_VALID_CM = 400;                  // typical max practical range

void Ultrasonic::begin(uint8_t trig, uint8_t echo, unsigned long timeoutUs) {
  trigPin = trig;
  echoPin = echo;
  this->timeoutUs = timeoutUs;
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void Ultrasonic::update() {
  const unsigned long nowMs = millis();
  if (nowMs - lastRead < MIN_INTERVAL_MS) return;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Use configured timeout but cap it to avoid long blocking.
  unsigned long t = timeoutUs;
  if (t == 0) t = MAX_PULSE_TIMEOUT_US;
  if (t > MAX_PULSE_TIMEOUT_US) t = MAX_PULSE_TIMEOUT_US;
  unsigned long duration = pulseIn(echoPin, HIGH, t);
  lastDurationUs = duration;

  if (duration != 0) {
    // distance(cm) ~= duration(us) / 58
    const int cm = (int)(duration / 58UL);
    if (cm >= MIN_VALID_CM && cm <= MAX_VALID_CM) {
      distance = cm;
      lastOk = true;
      lastRead = nowMs;
      return;
    }
  }

  // Invalid reading: keep last value briefly to reduce "999" flicker.
  if (lastOk && (nowMs - lastRead) <= VALID_HOLD_MS) {
    // keep previous distance and ok state
  } else {
    lastOk = false;
    distance = 999;
  }
  lastRead = nowMs;
}

int Ultrasonic::distanceCm() const {
  return distance;
}

unsigned long Ultrasonic::durationUs() const {
  return lastDurationUs;
}

bool Ultrasonic::ok() const {
  return lastOk;
}
