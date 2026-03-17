#include "state_sync.h"
#include "device_shadow.h"

void syncFromMega(const ParsedMsg& msg) {
  shadow.lastRxMs = millis();

  if (msg.key == "FAN1") shadow.fan1 = (msg.value.toInt() != 0);
  else if (msg.key == "FAN2") shadow.fan2 = (msg.value.toInt() != 0);
  else if (msg.key == "DOOR") {
    // Accept numeric (0/1) and string (OPEN/CLOSE) forms.
    if (msg.value == "OPEN") shadow.doorOpen = true;
    else if (msg.value == "CLOSE" || msg.value == "CLOSED") shadow.doorOpen = false;
    else shadow.doorOpen = (msg.value.toInt() != 0);
  }
  else if (msg.key == "LOCK") shadow.locked = (msg.value.toInt() != 0);
  else if (msg.key == "DORAW") shadow.doorRaw = msg.value.toInt();
  else if (msg.key == "DOORPOL") shadow.doorPol = msg.value.toInt();
  else if (msg.key == "T") shadow.tempC = msg.value.toFloat();
  else if (msg.key == "H") shadow.humidPct = msg.value.toFloat();
  else if (msg.key == "GAS") shadow.gas = msg.value.toInt();
  else if (msg.key == "GTH") shadow.gasThreshold = msg.value.toInt();
  else if (msg.key == "FIRE") shadow.fire = msg.value.toInt();
  else if (msg.key == "IR") shadow.ir = msg.value.toInt();
  else if (msg.key == "US") shadow.usCm = msg.value.toInt();
  else if (msg.key == "USOK") shadow.usOk = (msg.value.toInt() != 0);
  else if (msg.key == "PRES") shadow.pres = (msg.value.toInt() != 0);
  else if (msg.key == "LDR") shadow.ldr = msg.value.toInt();
  else if (msg.key == "DARK") shadow.ldrDark = (msg.value.toInt() != 0);
  else if (msg.key == "LTH") shadow.ldrThreshold = msg.value.toInt();
  else if (msg.key == "MODE") shadow.mode = msg.value;
  else if (msg.key == "LED") shadow.ledMode = msg.value;
  else if (msg.key == "UP") shadow.upSeconds = (unsigned long)msg.value.toInt();
  else if (msg.key == "RAM") shadow.freeRam = msg.value.toInt();
  else if (msg.key == "RSTF") shadow.resetFlags = msg.value.toInt();
  else if (msg.key == "RST") shadow.resetCause = msg.value;
  else if (msg.key == "EMG") shadow.emergency = (msg.value.toInt() != 0);
  else if (msg.key == "CUR") shadow.curtain = msg.value.toInt();
  else if (msg.key == "RBW") shadow.ledRainbow = (msg.value.toInt() != 0);
  else if (msg.key == "LEDM") shadow.ledMask = msg.value.toInt();
  else if (msg.key == "LC1") shadow.lc1 = msg.value.toInt();
  else if (msg.key == "LC2") shadow.lc2 = msg.value.toInt();
  else if (msg.key == "LC3") shadow.lc3 = msg.value.toInt();

  // Backward compatible keys (older ESP->Mega experiments)
  else if (msg.key == "FAN") shadow.fan1 = (msg.value == "ON" || msg.value.toInt() != 0);
}
