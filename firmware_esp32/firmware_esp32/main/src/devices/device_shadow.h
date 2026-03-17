#pragma once
#include <Arduino.h>

struct DeviceShadow {
  bool fan1;
  bool fan2;
  bool doorOpen;
  bool locked;
  int doorRaw;    // raw digital level from Mega door switch (0/1)
  int doorPol;    // configured open level on Mega (0=LOW, 1=HIGH)

  // Door command timestamps (ms) issued by ESP32 to Mega
  unsigned long lastDoorOpenCmdMs;
  unsigned long lastDoorCloseCmdMs;

  // Door voice timestamps (ms) to de-dup across multiple announcement paths
  unsigned long lastDoorOpenVoiceMs;
  unsigned long lastDoorCloseVoiceMs;

  // Curtain motion: -1 closing, 0 idle, 1 opening
  int curtain;

  // LED status from Mega
  bool ledRainbow;
  int ledMask; // bit0..2 = segment targets

  // LED color id per segment (0=WHITE,1=RED,2=GREEN,3=BLUE)
  int lc1;
  int lc2;
  int lc3;

  // Sensors
  float tempC;
  float humidPct;
  int gas;
  int gasThreshold;
  int fire;
  int ir;
  int usCm;
  bool usOk;
  bool pres;
  int ldr;
  bool ldrDark;
  int ldrThreshold;

  // System / diagnostics
  unsigned long upSeconds;
  int freeRam;
  int resetFlags;
  String resetCause;
  bool emergency;

  // Meta
  unsigned long lastRxMs;

  String mode;
  String ledMode;
};

extern DeviceShadow shadow;
