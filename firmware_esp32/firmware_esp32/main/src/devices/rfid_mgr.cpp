#include "rfid_mgr.h"

#include <SPI.h>
#include <MFRC522.h>

#include "../core/config.h"

static MFRC522 g_rfid(RFID_SS_PIN, RFID_RST_PIN);

void RFIDMgr::begin() {
  SPI.begin();
  g_rfid.PCD_Init();
  inited_ = true;
}

bool RFIDMgr::pollUID(char* outUid, size_t outLen) {
  if (!inited_) return false;
  if (!outUid || outLen == 0) return false;

  if (!g_rfid.PICC_IsNewCardPresent()) return false;
  if (!g_rfid.PICC_ReadCardSerial()) return false;

  // UID as uppercase hex, no separators
  char buf[32] = {0};
  size_t pos = 0;

  for (byte i = 0; i < g_rfid.uid.size; i++) {
    const byte b = g_rfid.uid.uidByte[i];
    const char* hex = "0123456789ABCDEF";
    if (pos + 2 >= sizeof(buf)) break;
    buf[pos++] = hex[(b >> 4) & 0x0F];
    buf[pos++] = hex[b & 0x0F];
  }
  buf[pos] = '\0';

  strncpy(outUid, buf, outLen - 1);
  outUid[outLen - 1] = '\0';

  g_rfid.PICC_HaltA();
  g_rfid.PCD_StopCrypto1();

  return true;
}
