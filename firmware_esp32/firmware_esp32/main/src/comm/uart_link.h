#pragma once
#include <HardwareSerial.h>

class UARTLink {
public:
  void begin();
  void send(const String& msg);
  // Non-blocking: returns true only when a full packet (terminated by ';') is received.
  bool tryReadPacket(String& out);

private:
  static const size_t RX_BUF_SIZE = 128;
  char rxBuf[RX_BUF_SIZE] = {0};
  size_t rxIdx = 0;
};
