#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <Arduino.h>
#include <string.h>

#define UART_BAUDRATE 9600
#define UART_BUFFER_SIZE 128

enum UartCommand {
  CMD_UNKNOWN,
  CMD_FAN1_ON,
  CMD_FAN1_OFF,
  CMD_FAN2_ON,
  CMD_FAN2_OFF,
  CMD_DOOR_OPEN,
  CMD_DOOR_CLOSE,
  CMD_LOCK,
  CMD_UNLOCK,
  CMD_CURTAIN_OPEN,
  CMD_CURTAIN_CLOSE,
  CMD_CURTAIN_STOP,

  // LED commands (parsed in uart_protocol.cpp)
  CMD_LED_ZONE,
  CMD_LED_ALL,
  CMD_LED_OFF,
  CMD_LED_RAINBOW,

  // LED segment commands (3 segments -> 3 lights)
  CMD_SEG1_ON,
  CMD_SEG1_OFF,
  CMD_SEG2_ON,
  CMD_SEG2_OFF,
  CMD_SEG3_ON,
  CMD_SEG3_OFF,

  // Fixed colors
  CMD_SEG1_WHITE,
  CMD_SEG1_RED,
  CMD_SEG1_GREEN,
  CMD_SEG1_BLUE,
  CMD_SEG1_YELLOW,
  CMD_SEG1_PURPLE,
  CMD_SEG2_WHITE,
  CMD_SEG2_RED,
  CMD_SEG2_GREEN,
  CMD_SEG2_BLUE,
  CMD_SEG2_YELLOW,
  CMD_SEG2_PURPLE,
  CMD_SEG3_WHITE,
  CMD_SEG3_RED,
  CMD_SEG3_GREEN,
  CMD_SEG3_BLUE,
  CMD_SEG3_YELLOW,
  CMD_SEG3_PURPLE,

  CMD_ALL_WHITE,
  CMD_ALL_RED,
  CMD_ALL_GREEN,
  CMD_ALL_BLUE,
  CMD_ALL_YELLOW,
  CMD_ALL_PURPLE

  ,
  // RFID UI commands from ESP32
  CMD_RFID_CHECK,
  CMD_RFID_FAIL,
  CMD_RFID_HELLO,

  // Simple buzzer feedback (ESP32 -> Mega)
  CMD_BEEP,

  // Hostage/emergency PIN signaling from ESP32
  CMD_HOSTAGE_ON,
  CMD_HOSTAGE_BEEP,
  CMD_HOSTAGE_SIREN,
  CMD_HOSTAGE_CLEAR,

  // Keypad/admin acknowledgements from ESP32
  CMD_PIN_OK,
  CMD_PIN_FAIL,
  CMD_ADMIN_OK,
  CMD_ADMIN_FAIL
};

class UartProtocol {
public:
  void begin(HardwareSerial& serial);
  void update();

  bool hasCommand() const;
  UartCommand getCommand();

  // Optional argument for commands like CMD_RFID_HELLO.
  const char* getArg() const { return lastArg; }

  void sendKV(const char* key, const char* value);
  void sendKV(const char* key, int value);
  void sendKV(const char* key, unsigned long value);
  void sendKV(const char* key, float value, uint8_t decimals = 1);
  void sendStatus(const char* msg);

  // Send a command packet to ESP32: "CMD:<cmd>;"
  void sendCmd(const char* cmd);

private:
  HardwareSerial* uart;
  char buffer[UART_BUFFER_SIZE];
  uint8_t index = 0;

  UartCommand lastCommand = CMD_UNKNOWN;

  char lastArg[33] = {0};

  void parseLine(const char* line);
};

#endif
