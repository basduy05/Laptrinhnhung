#include "uart_protocol.h"

void UartProtocol::begin(HardwareSerial& serial) {
  uart = &serial;
  uart->begin(UART_BAUDRATE);
}

void UartProtocol::update() {
  while (uart->available()) {
    char c = uart->read();

    if (c == ';') {
      buffer[index] = '\0';
      parseLine(buffer);
      index = 0;
    } else if (index < UART_BUFFER_SIZE - 1) {
      buffer[index++] = c;
    }
  }
}

void UartProtocol::parseLine(const char* line) {
  lastCommand = CMD_UNKNOWN;
  lastArg[0] = '\0';

  if (strncmp(line, "CMD:", 4) != 0) return;

  const char* cmd = line + 4;

  // RFID UI commands (ESP32 -> Mega)
  if (strcmp(cmd, "RFID_CHECK") == 0) {
    lastCommand = CMD_RFID_CHECK;
    return;
  }
  if (strcmp(cmd, "RFID_FAIL") == 0) {
    lastCommand = CMD_RFID_FAIL;
    return;
  }
  if (strcmp(cmd, "BEEP") == 0) {
    lastCommand = CMD_BEEP;
    return;
  }
  if (strcmp(cmd, "HOSTAGE_ON") == 0) {
    lastCommand = CMD_HOSTAGE_ON;
    return;
  }
  if (strcmp(cmd, "HOSTAGE_BEEP") == 0) {
    lastCommand = CMD_HOSTAGE_BEEP;
    return;
  }
  if (strcmp(cmd, "HOSTAGE_SIREN") == 0) {
    lastCommand = CMD_HOSTAGE_SIREN;
    return;
  }
  if (strcmp(cmd, "HOSTAGE_CLEAR") == 0) {
    lastCommand = CMD_HOSTAGE_CLEAR;
    return;
  }
  if (strcmp(cmd, "PIN_OK") == 0) {
    lastCommand = CMD_PIN_OK;
    return;
  }
  if (strcmp(cmd, "PIN_FAIL") == 0) {
    lastCommand = CMD_PIN_FAIL;
    return;
  }
  if (strcmp(cmd, "ADMIN_OK") == 0) {
    lastCommand = CMD_ADMIN_OK;
    return;
  }
  if (strcmp(cmd, "ADMIN_FAIL") == 0) {
    lastCommand = CMD_ADMIN_FAIL;
    return;
  }
  if (strncmp(cmd, "HELLO:", 6) == 0) {
    lastCommand = CMD_RFID_HELLO;
    strncpy(lastArg, cmd + 6, sizeof(lastArg) - 1);
    lastArg[sizeof(lastArg) - 1] = '\0';
    return;
  }

  if      (strcmp(cmd, "FAN1_ON") == 0) lastCommand = CMD_FAN1_ON;
  else if (strcmp(cmd, "FAN1_OFF") == 0) lastCommand = CMD_FAN1_OFF;
  else if (strcmp(cmd, "FAN2_ON") == 0) lastCommand = CMD_FAN2_ON;
  else if (strcmp(cmd, "FAN2_OFF") == 0) lastCommand = CMD_FAN2_OFF;
  else if (strcmp(cmd, "DOOR_OPEN") == 0) lastCommand = CMD_DOOR_OPEN;
  else if (strcmp(cmd, "DOOR_CLOSE") == 0) lastCommand = CMD_DOOR_CLOSE;
  else if (strcmp(cmd, "LOCK") == 0) lastCommand = CMD_LOCK;
  else if (strcmp(cmd, "UNLOCK") == 0) lastCommand = CMD_UNLOCK;
  else if (strcmp(cmd, "CURTAIN_OPEN") == 0) lastCommand = CMD_CURTAIN_OPEN;
  else if (strcmp(cmd, "CURTAIN_CLOSE") == 0) lastCommand = CMD_CURTAIN_CLOSE;
  else if (strcmp(cmd, "CURTAIN_STOP") == 0) lastCommand = CMD_CURTAIN_STOP;
  else if (strncmp(cmd, "LED_ZONE", 8) == 0) lastCommand = CMD_LED_ZONE;
  else if (strncmp(cmd, "LED_ALL", 7) == 0) lastCommand = CMD_LED_ALL;
  else if (strcmp(cmd, "LED_OFF") == 0) lastCommand = CMD_LED_OFF;
  else if (strcmp(cmd, "LED_RAINBOW") == 0) lastCommand = CMD_LED_RAINBOW;
  else if (strcmp(cmd, "SEG1_ON") == 0) lastCommand = CMD_SEG1_ON;
  else if (strcmp(cmd, "SEG1_OFF") == 0) lastCommand = CMD_SEG1_OFF;
  else if (strcmp(cmd, "SEG2_ON") == 0) lastCommand = CMD_SEG2_ON;
  else if (strcmp(cmd, "SEG2_OFF") == 0) lastCommand = CMD_SEG2_OFF;
  else if (strcmp(cmd, "SEG3_ON") == 0) lastCommand = CMD_SEG3_ON;
  else if (strcmp(cmd, "SEG3_OFF") == 0) lastCommand = CMD_SEG3_OFF;
  else if (strcmp(cmd, "SEG1_WHITE") == 0) lastCommand = CMD_SEG1_WHITE;
  else if (strcmp(cmd, "SEG1_RED") == 0) lastCommand = CMD_SEG1_RED;
  else if (strcmp(cmd, "SEG1_GREEN") == 0) lastCommand = CMD_SEG1_GREEN;
  else if (strcmp(cmd, "SEG1_BLUE") == 0) lastCommand = CMD_SEG1_BLUE;
  else if (strcmp(cmd, "SEG1_YELLOW") == 0) lastCommand = CMD_SEG1_YELLOW;
  else if (strcmp(cmd, "SEG1_PURPLE") == 0) lastCommand = CMD_SEG1_PURPLE;
  else if (strcmp(cmd, "SEG2_WHITE") == 0) lastCommand = CMD_SEG2_WHITE;
  else if (strcmp(cmd, "SEG2_RED") == 0) lastCommand = CMD_SEG2_RED;
  else if (strcmp(cmd, "SEG2_GREEN") == 0) lastCommand = CMD_SEG2_GREEN;
  else if (strcmp(cmd, "SEG2_BLUE") == 0) lastCommand = CMD_SEG2_BLUE;
  else if (strcmp(cmd, "SEG2_YELLOW") == 0) lastCommand = CMD_SEG2_YELLOW;
  else if (strcmp(cmd, "SEG2_PURPLE") == 0) lastCommand = CMD_SEG2_PURPLE;
  else if (strcmp(cmd, "SEG3_WHITE") == 0) lastCommand = CMD_SEG3_WHITE;
  else if (strcmp(cmd, "SEG3_RED") == 0) lastCommand = CMD_SEG3_RED;
  else if (strcmp(cmd, "SEG3_GREEN") == 0) lastCommand = CMD_SEG3_GREEN;
  else if (strcmp(cmd, "SEG3_BLUE") == 0) lastCommand = CMD_SEG3_BLUE;
  else if (strcmp(cmd, "SEG3_YELLOW") == 0) lastCommand = CMD_SEG3_YELLOW;
  else if (strcmp(cmd, "SEG3_PURPLE") == 0) lastCommand = CMD_SEG3_PURPLE;
  else if (strcmp(cmd, "ALL_WHITE") == 0) lastCommand = CMD_ALL_WHITE;
  else if (strcmp(cmd, "ALL_RED") == 0) lastCommand = CMD_ALL_RED;
  else if (strcmp(cmd, "ALL_GREEN") == 0) lastCommand = CMD_ALL_GREEN;
  else if (strcmp(cmd, "ALL_BLUE") == 0) lastCommand = CMD_ALL_BLUE;
  else if (strcmp(cmd, "ALL_YELLOW") == 0) lastCommand = CMD_ALL_YELLOW;
  else if (strcmp(cmd, "ALL_PURPLE") == 0) lastCommand = CMD_ALL_PURPLE;
}

bool UartProtocol::hasCommand() const {
  return lastCommand != CMD_UNKNOWN;
}

UartCommand UartProtocol::getCommand() {
  UartCommand cmd = lastCommand;
  lastCommand = CMD_UNKNOWN;
  return cmd;
}

void UartProtocol::sendKV(const char* key, const char* value) {
  uart->print(key);
  uart->print(':');
  uart->print(value);
  uart->print(';');
}

void UartProtocol::sendKV(const char* key, int value) {
  uart->print(key);
  uart->print(':');
  uart->print(value);
  uart->print(';');
}

void UartProtocol::sendKV(const char* key, unsigned long value) {
  uart->print(key);
  uart->print(':');
  uart->print(value);
  uart->print(';');
}

void UartProtocol::sendKV(const char* key, float value, uint8_t decimals) {
  uart->print(key);
  uart->print(':');
  uart->print(value, decimals);
  uart->print(';');
}

void UartProtocol::sendStatus(const char* msg) {
  uart->print("STATE:");
  uart->print(msg);
  uart->print(';');
}

void UartProtocol::sendCmd(const char* cmd) {
  uart->print("CMD:");
  uart->print(cmd);
  uart->print(';');
}
