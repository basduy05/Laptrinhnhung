#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Adafruit_NeoPixel.h>

#define LED_ZONE_COUNT 3
#define LEDS_PER_ZONE  6

enum LedMode {
  LED_OFF,
  LED_SOLID,
  LED_RAINBOW
};

class StatusLED {
public:
  StatusLED();
  void begin(uint8_t pin, uint16_t totalLeds);

  void setZoneColor(uint8_t zone, uint8_t r, uint8_t g, uint8_t b);
  void setZoneOff(uint8_t zone);

  void setAllColor(uint8_t r, uint8_t g, uint8_t b);
  void setAllOff();

  void enableRainbow(bool enable);
  void update();

  // Reference-sketch LED features
  void setSegmentTarget(uint8_t segment, bool on);
  void toggleSegment(uint8_t segment);
  void setAllSegmentsTarget(bool on);

  // Immediate clear (no fade). Useful for strict "queue" behavior.
  void hardOffSegment(uint8_t segment);
  void hardOffAll();

  // 0=WHITE, 1=RED, 2=GREEN, 3=BLUE
  void setSegmentColorId(uint8_t segment, uint8_t colorId);
  void setAllSegmentsColorId(uint8_t colorId);
  uint8_t segmentColorId(uint8_t segment) const;
  void setRainbowTarget(bool on);
  void toggleRainbowTarget();
  bool rainbowTarget() const;
  bool anySegmentTarget() const;
  uint8_t segmentMask() const;
  void allOff();

  void setEmergency(bool active);

private:
  Adafruit_NeoPixel strip;

  bool renderDirty_ = false;

  // Backward-compatible (zones map to segments internally)
  LedMode mode = LED_OFF;
  uint32_t zoneColor[LED_ZONE_COUNT];
  bool zoneEnabled[LED_ZONE_COUNT];

  // Effects state (ported from your stable sketch)
  bool segmentTarget[LED_ZONE_COUNT] = {false, false, false};
  float segmentCurrentBright[LED_ZONE_COUNT] = {0.0f, 0.0f, 0.0f};

  uint8_t segmentColorId_[LED_ZONE_COUNT] = {0, 0, 0};

  bool targetRainbow = false;
  float currentRainbowBright = 0.0f;
  long rainbowPixelHue = 0;

  bool emergencyActive = false;
  bool previousEmergency = false;
  enum PostEmergencyPhase { POST_NONE, POST_HOLD, POST_FADE };
  PostEmergencyPhase postEmergencyPhase = POST_NONE;
  unsigned long postEmergencyStart = 0;
  float emergencyLedBrightness = 255.0f;

  unsigned long lastUpdate = 0;
  unsigned long lastBlink = 0;
  bool blinkToggle = false;

  void applyZones();
  void handleEmergencyLed();
  void showSolidEmergency(uint8_t brightness);
  bool fadeOutEmergencyLed();
  void handleLedEffects();
};

#endif
