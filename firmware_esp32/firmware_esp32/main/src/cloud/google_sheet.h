#pragma once

#include <Arduino.h>

struct GoogleLogResult {
	char uid[24];
	int httpCode;      // 200 = OK, negative for local errors
	bool ok;
	char name[32];
	char message[96];
};

struct GoogleLogEvent {
	char uid[24];
	char name[32];
	char status[8];   // IN / OUT / DENY
	long epoch;       // seconds
};

class GoogleSheetLogger {
public:
	void begin();

	// Enqueue an event for Google Sheet logging.
	bool enqueueEvent(const GoogleLogEvent& ev);

	// Non-blocking: get a finished result.
	bool tryDequeueResult(GoogleLogResult& out);

private:
	void ensureTaskStarted_();

	static void taskFn_(void* arg);

	void* uidQueue_ = nullptr;
	void* resultQueue_ = nullptr;
	bool started_ = false;
};

