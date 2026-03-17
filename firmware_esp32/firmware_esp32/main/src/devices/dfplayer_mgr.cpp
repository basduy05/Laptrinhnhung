#include "dfplayer_mgr.h"

#include "../core/config.h"

void DFPlayerMgr::begin() {
  mp3Serial_.begin(DFP_BAUD, SERIAL_8N1, DFP_RX_PIN, DFP_TX_PIN);
  // DFPlayer often needs a moment after power-up.
  delay(1200);

  ready_ = false;
  Serial.printf("[DFP] begin: baud=%d rx=%d tx=%d\r\n", (int)DFP_BAUD, (int)DFP_RX_PIN, (int)DFP_TX_PIN);

  // Try a few times. Some modules respond only after a reset handshake.
  for (int attempt = 1; attempt <= 5 && !ready_; attempt++) {
    const bool doReset = (attempt == 1);
    const bool ok = player_.begin(mp3Serial_, /*isACK=*/true, /*doReset=*/doReset);
    Serial.printf("[DFP] attempt %d -> %s (reset=%d)\r\n", attempt, ok ? "OK" : "FAIL", (int)doReset);
    if (ok) {
      ready_ = true;
      player_.outputDevice(DFPLAYER_DEVICE_SD);
      player_.volume(volume_);
      break;
    }
    delay(600);
  }

  if (!ready_) {
    Serial.println("[DFP] not ready (check wiring, SD card, speaker)");
  }
}

void DFPlayerMgr::setVolume(uint8_t vol) {
  volume_ = vol;
  if (ready_) player_.volume(volume_);
}

void DFPlayerMgr::play(uint16_t track) {
  if (!ready_) return;
  // Use /mp3/0001.mp3 numbering for stable playback order.
  Serial.printf("[DFP] play track=%u\r\n", (unsigned)track);
  player_.playMp3Folder((int)track);
}
