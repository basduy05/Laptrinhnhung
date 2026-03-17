#pragma once

#include <Arduino.h>

// Initializes gateway state (RFID lists, timestamps, etc.)
void gatewayBegin();

// Non-blocking: reads commands from USB Serial and emits ack/event/state JSON.
void gatewaySerialLoop();

// Non-blocking: polls RFID hardware and emits rfid_scan event (and optional door open).
void gatewayRfidLoop();

// Emits periodic JSON state to USB Serial (for bridge polling).
void gatewayStateLoop();

// HTTP helpers (return JSON as String)
String gatewayBuildSensorsJson(bool includeType = false);
String gatewayBuildCapsJson();

String gatewayBuildRfidLastJson();
String gatewayBuildRfidCardsJson();
String gatewayBuildRfidHistoryJson();

String gatewayBuildPinsJson();
String gatewayBuildEmergencyPinsJson();

// Add/update a card. Name is optional but recommended.
bool gatewayRfidAddCard(const String& uid, const String& name = String());
bool gatewayRfidDelCard(const String& uid);

// PIN/password management
bool gatewayPinAdd(const String& pin);
bool gatewayPinDel(const String& pin);

// Emergency PIN (hostage) management
bool gatewayEmergencyPinAdd(const String& pin);
bool gatewayEmergencyPinDel(const String& pin);

// Record an RFID scan result decided by external authority (e.g., Google Sheet).
// This updates last-scan + history for web/UI without using the local authorized list.
// - uid: uppercase hex UID
// - authorized: true if valid
// - name: optional display name (may be empty)
// - direction: "IN"/"OUT" when authorized, or "" when denied
// - epoch: seconds (0 allowed if time not ready)
void gatewayRfidRecordScan(const char* uid, bool authorized, const char* name, const char* direction, long epoch);

struct GatewayRfidLast {
	char uid[24];
	char name[32];
	bool authorized;
	// "IN" for enter (door open), "OUT" for exit (door close), or "" if unknown.
	char direction[8];
	unsigned long tsMs;
	long epoch;
};

// Non-blocking snapshot of last scan for UI/logging.
bool gatewayGetLastRfid(GatewayRfidLast& out);

// Execute a command string coming from web/serial/voice.
// Returns true if accepted. If false, `errOut` describes error.
bool gatewayHandleCommand(const String& cmd, String& errOut, const char* source);
