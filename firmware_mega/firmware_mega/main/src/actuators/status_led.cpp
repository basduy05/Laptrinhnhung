#include "status_led.h"

StatusLED::StatusLED() : strip(18, 7, NEO_GRB + NEO_KHZ800) {}

void StatusLED::begin(uint8_t pin, uint16_t totalLeds) {
  strip = Adafruit_NeoPixel(totalLeds, pin, NEO_GRB + NEO_KHZ800);
  strip.begin();
  strip.clear();
  strip.show();

  renderDirty_ = true;

  for (int i = 0; i < LED_ZONE_COUNT; i++) {
    zoneEnabled[i] = false;
    zoneColor[i] = strip.Color(0, 0, 0);
    segmentTarget[i] = false;
    segmentCurrentBright[i] = 0.0f;
    segmentColorId_[i] = 0;
  }

  targetRainbow = false;
  currentRainbowBright = 0.0f;
  rainbowPixelHue = 0;

  emergencyActive = false;
  previousEmergency = false;
  postEmergencyPhase = POST_NONE;
  postEmergencyStart = 0;
  emergencyLedBrightness = 255.0f;

  lastUpdate = 0;
  lastBlink = 0;
  blinkToggle = false;
}

void StatusLED::setZoneColor(uint8_t zone, uint8_t r, uint8_t g, uint8_t b) {
  if (zone >= LED_ZONE_COUNT) return;
  zoneColor[zone] = strip.Color(r, g, b);
  zoneEnabled[zone] = true;
  mode = LED_SOLID;

  // Also reflect to segment targets
  segmentTarget[zone] = true;
  renderDirty_ = true;
}

void StatusLED::setZoneOff(uint8_t zone) {
  if (zone >= LED_ZONE_COUNT) return;
  zoneEnabled[zone] = false;
  segmentTarget[zone] = false;
  renderDirty_ = true;
}

void StatusLED::setAllColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < LED_ZONE_COUNT; i++) {
    zoneColor[i] = strip.Color(r, g, b);
    zoneEnabled[i] = true;
    segmentTarget[i] = true;
  }
  mode = LED_SOLID;
  renderDirty_ = true;
}

void StatusLED::setAllOff() {
  allOff();
}

void StatusLED::enableRainbow(bool enable) {
  setRainbowTarget(enable);
}

void StatusLED::update() {
  const unsigned long now = millis();
  if (now - lastUpdate < 30UL) return; // smoother fades, still avoids hammering the CPU
  lastUpdate = now;

  // Emergency blink + post-emergency hold/fade
  if (emergencyActive) {
    handleEmergencyLed();
    postEmergencyPhase = POST_NONE;
    previousEmergency = true;
    return;
  }

  if (previousEmergency && postEmergencyPhase == POST_NONE) {
    postEmergencyPhase = POST_HOLD;
    postEmergencyStart = now;
    emergencyLedBrightness = 255.0f; // Restore full brightness as only 6 LEDs are used
    showSolidEmergency((uint8_t)emergencyLedBrightness);
  }

  if (postEmergencyPhase == POST_HOLD && (now - postEmergencyStart) >= 4000UL) {
    postEmergencyPhase = POST_FADE;
  }

  if (postEmergencyPhase == POST_FADE) {
    if (fadeOutEmergencyLed()) {
      postEmergencyPhase = POST_NONE;
      strip.clear();
      strip.show();
      previousEmergency = false;

      // Reset brightness to avoid sudden jump
      for (int i = 0; i < LED_ZONE_COUNT; i++) segmentCurrentBright[i] = 0.0f;
      currentRainbowBright = 0.0f;
    }
    return;
  }

  handleLedEffects();
}

void StatusLED::applyZones() {
  for (int z = 0; z < LED_ZONE_COUNT; z++) {
    for (int i = 0; i < LEDS_PER_ZONE; i++) {
      int idx = z * LEDS_PER_ZONE + i;
      strip.setPixelColor(idx,
        zoneEnabled[z] ? zoneColor[z] : 0);
    }
  }
}

void StatusLED::setSegmentTarget(uint8_t segment, bool on) {
  if (segment >= LED_ZONE_COUNT) return;
  segmentTarget[segment] = on;
  renderDirty_ = true;
}

void StatusLED::toggleSegment(uint8_t segment) {
  if (segment >= LED_ZONE_COUNT) return;
  segmentTarget[segment] = !segmentTarget[segment];
  renderDirty_ = true;
}

void StatusLED::setAllSegmentsTarget(bool on) {
  for (int i = 0; i < LED_ZONE_COUNT; i++) segmentTarget[i] = on;
  renderDirty_ = true;
}

void StatusLED::hardOffSegment(uint8_t segment) {
  if (segment >= LED_ZONE_COUNT) return;
  segmentTarget[segment] = false;
  segmentCurrentBright[segment] = 0.0f;

  const int start = (int)segment * LEDS_PER_ZONE;
  const int end = start + LEDS_PER_ZONE;
  for (int p = start; p < end; p++) {
    strip.setPixelColor(p, 0);
  }
  strip.show();
  renderDirty_ = false;
}

void StatusLED::hardOffAll() {
  targetRainbow = false;
  currentRainbowBright = 0.0f;

  setAllSegmentsTarget(false);
  for (int i = 0; i < LED_ZONE_COUNT; i++) {
    segmentCurrentBright[i] = 0.0f;
  }

  strip.clear();
  strip.show();
  renderDirty_ = false;
}

void StatusLED::setSegmentColorId(uint8_t segment, uint8_t colorId) {
  if (segment >= LED_ZONE_COUNT) return;
  if (colorId > 5) colorId = 0;
  segmentColorId_[segment] = colorId;
  renderDirty_ = true;
}

void StatusLED::setAllSegmentsColorId(uint8_t colorId) {
  if (colorId > 5) colorId = 0;
  for (int i = 0; i < LED_ZONE_COUNT; i++) segmentColorId_[i] = colorId;
  renderDirty_ = true;
}

uint8_t StatusLED::segmentColorId(uint8_t segment) const {
  if (segment >= LED_ZONE_COUNT) return 0;
  return segmentColorId_[segment];
}

void StatusLED::setRainbowTarget(bool on) {
  targetRainbow = on;
  if (on) {
    // Avoid conflict between rainbow and static segments
    setAllSegmentsTarget(false);
  }
  renderDirty_ = true;
}

void StatusLED::toggleRainbowTarget() {
  setRainbowTarget(!targetRainbow);
}

bool StatusLED::rainbowTarget() const {
  return targetRainbow;
}

bool StatusLED::anySegmentTarget() const {
  return segmentTarget[0] || segmentTarget[1] || segmentTarget[2];
}

uint8_t StatusLED::segmentMask() const {
  uint8_t m = 0;
  if (segmentTarget[0]) m |= 0x01;
  if (segmentTarget[1]) m |= 0x02;
  if (segmentTarget[2]) m |= 0x04;
  return m;
}

void StatusLED::allOff() {
  targetRainbow = false;
  setAllSegmentsTarget(false);
  renderDirty_ = true;
}

void StatusLED::setEmergency(bool active) {
  emergencyActive = active;
}

void StatusLED::handleEmergencyLed() {
  const unsigned long now = millis();
  static const uint8_t emergencyIndices[] = {0, 5, 6, 11, 12, 17};

  if (now - lastBlink > 200UL) {
    lastBlink = now;
    blinkToggle = !blinkToggle;
    strip.clear();
    if (blinkToggle) {
      for (uint8_t i = 0; i < (uint8_t)(sizeof(emergencyIndices) / sizeof(emergencyIndices[0])); i++) {
        strip.setPixelColor(emergencyIndices[i], strip.Color(255, 0, 0));
      }
    }
    strip.show();
  }
}

void StatusLED::showSolidEmergency(uint8_t brightness) {
  static const uint8_t emergencyIndices[] = {0, 5, 6, 11, 12, 17};
  for (uint8_t i = 0; i < (uint8_t)(sizeof(emergencyIndices) / sizeof(emergencyIndices[0])); i++) {
    strip.setPixelColor(emergencyIndices[i], strip.Color(brightness, 0, 0));
  }
  strip.show();
}

bool StatusLED::fadeOutEmergencyLed() {
  if (emergencyLedBrightness > 0.0f) {
    emergencyLedBrightness -= 1.2f;
    if (emergencyLedBrightness < 0.0f) emergencyLedBrightness = 0.0f;
    showSolidEmergency((uint8_t)emergencyLedBrightness);
    return false;
  }
  return true;
}

void StatusLED::handleLedEffects() {
  bool needUpdate = false;

  // Tunables (faster than previous)
  static const float FADE_IN_STEP = 2.5f;
  static const float FADE_OUT_STEP = 2.0f;

  // RAINBOW fade in/out
  if (targetRainbow && currentRainbowBright < 80.0f) {
    currentRainbowBright += FADE_IN_STEP;
    needUpdate = true;
  } else if (!targetRainbow && currentRainbowBright > 0.0f) {
    currentRainbowBright -= FADE_OUT_STEP;
    if (currentRainbowBright < 0.0f) currentRainbowBright = 0.0f;
    needUpdate = true;
  }

  // If rainbow visible enough, render rainbow and exit
  if (currentRainbowBright > 0.5f) {
    rainbowPixelHue += 256;
    const float rainbowFactor = currentRainbowBright / 255.0f;
    for (int i = 0; i < strip.numPixels(); i++) {
      long hue = rainbowPixelHue + (long)i * 65536L / strip.numPixels();
      uint32_t color = strip.gamma32(strip.ColorHSV((uint16_t)hue));
      uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * rainbowFactor);
      uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * rainbowFactor);
      uint8_t b = (uint8_t)((color & 0xFF) * rainbowFactor);
      strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
    return;
  }

  // SEGMENTS fade in/out
  for (int seg = 0; seg < LED_ZONE_COUNT; seg++) {
    if (segmentTarget[seg] && segmentCurrentBright[seg] < 80.0f) {
      segmentCurrentBright[seg] += FADE_IN_STEP;
      needUpdate = true;
    } else if (!segmentTarget[seg] && segmentCurrentBright[seg] > 0.0f) {
      segmentCurrentBright[seg] -= FADE_OUT_STEP;
      if (segmentCurrentBright[seg] < 0.0f) segmentCurrentBright[seg] = 0.0f;
      needUpdate = true;
    }

    const int start = seg * LEDS_PER_ZONE;
    const int end = start + LEDS_PER_ZONE;
    const float factor = segmentCurrentBright[seg] / 255.0f;

    // Default WHITE tuned to avoid yellow cast on many strips.
    uint8_t baseR = 230, baseG = 230, baseB = 255;
    switch (segmentColorId_[seg]) {
      case 1: baseR = 255; baseG = 0; baseB = 0; break;   // RED
      case 2: baseR = 0; baseG = 255; baseB = 0; break;   // GREEN
      case 3: baseR = 0; baseG = 0; baseB = 255; break;   // BLUE
      case 4: baseR = 255; baseG = 200; baseB = 0; break; // YELLOW
      case 5: baseR = 160; baseG = 0; baseB = 255; break; // PURPLE
      default: break;
    }

    for (int p = start; p < end; p++) {
      uint8_t r = (uint8_t)(baseR * factor);
      uint8_t g = (uint8_t)(baseG * factor);
      uint8_t b = (uint8_t)(baseB * factor);
      strip.setPixelColor(p, strip.Color(r, g, b));
    }
  }

  if (needUpdate || renderDirty_) {
    strip.show();
    renderDirty_ = false;
  }
}
