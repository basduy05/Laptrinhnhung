#pragma once

#define WIFI_SSID "potata"
#define WIFI_PASS "12345678@"

// SoftAP fallback (optional but recommended for local control)
#define WIFI_AP_SSID "Miki-SmartHome"
#define WIFI_AP_PASS "12345678"

// Google Apps Script deployment id
#define GSCRIPT_ID "AKfycbxz2tgbEJ9LAwrB0NqsuspS-7NuaTkpseA4ik7VhEpfCJ7aKV4HOquJ3gp0uMPbySUy"

#define UART_BAUD 9600
#define UART_TX   17
#define UART_RX   16

#define HEARTBEAT_INTERVAL 3000

// ===== ESP32 Devices =====

// RC522 (MFRC522) - SPI
#define RFID_SS_PIN  5
#define RFID_RST_PIN 4

// LCD 2004 I2C
#define LCD_I2C_ADDR 0x27

// DFPlayer Mini (separate UART from Mega link)
#define DFP_BAUD   9600
// TX DFPlayer -> RX ESP32
#define DFP_RX_PIN 32
// RX DFPlayer -> TX ESP32
#define DFP_TX_PIN 33
