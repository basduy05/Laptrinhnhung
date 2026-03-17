#include "uart_link.h"
#include "../core/config.h"

HardwareSerial Mega(2);

void UARTLink::begin() {
  Mega.begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);
}

void UARTLink::send(const String& msg) {
  Mega.print(msg);
  if (!msg.endsWith(";")) {
    Mega.print(';');
  }
}

bool UARTLink::tryReadPacket(String& out) {
  while (Mega.available()) {
    const char c = (char)Mega.read();

    // Ignore CR/LF (Mega side doesn't send them, but safe)
    if (c == '\r' || c == '\n') {
      continue;
    }

    if (c == ';') {
      rxBuf[rxIdx] = '\0';
      out = String(rxBuf);
      rxIdx = 0;
      return true;
    }

    if (rxIdx < RX_BUF_SIZE - 1) {
      rxBuf[rxIdx++] = c;
    } else {
      // Overflow: reset buffer to avoid desync
      rxIdx = 0;
    }
  }

  return false;
}
