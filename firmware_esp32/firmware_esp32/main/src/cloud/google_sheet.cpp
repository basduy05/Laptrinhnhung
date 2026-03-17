#include "google_sheet.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include "../core/config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

namespace {

struct UidMsg {
	GoogleLogEvent ev;
};

static bool isUnreserved_(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~';
}

static String urlEncode_(const char* s) {
	if (!s) return "";
	String out;
	for (size_t i = 0; s[i] != '\0'; i++) {
		const char c = s[i];
		if (isUnreserved_(c)) {
			out += c;
		} else if (c == ' ') {
			out += "%20";
		} else {
			char buf[4];
			snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
			out += buf;
		}
	}
	return out;
}

} // namespace

void GoogleSheetLogger::taskFn_(void* arg) {
	GoogleSheetLogger* self = (GoogleSheetLogger*)arg;
	if (!self) {
		vTaskDelete(nullptr);
		return;
	}

	QueueHandle_t uidQ = (QueueHandle_t)self->uidQueue_;
	QueueHandle_t resQ = (QueueHandle_t)self->resultQueue_;

	for (;;) {
		UidMsg msg;
		if (xQueueReceive(uidQ, &msg, portMAX_DELAY) != pdTRUE) {
			continue;
		}

		GoogleLogResult result;
		memset(&result, 0, sizeof(result));
		strncpy(result.uid, msg.ev.uid, sizeof(result.uid) - 1);
		result.ok = false;
		result.name[0] = 0;
		result.message[0] = 0;

		if (WiFi.status() != WL_CONNECTED) {
			result.httpCode = -10; // no wifi
			xQueueSend(resQ, &result, 0);
			continue;
		}

		WiFiClientSecure client;
		client.setInsecure();
		client.setTimeout(15000);

		HTTPClient http;
		http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
		const String base = String("https://script.google.com/macros/s/") + GSCRIPT_ID + "/exec";

		const String qs = String("uid=") + urlEncode_(msg.ev.uid)
			+ "&name=" + urlEncode_(msg.ev.name)
			+ "&status=" + urlEncode_(msg.ev.status)
			+ "&epoch=" + String(msg.ev.epoch);

		const String url = base + "?" + qs;

		int code = -20; // begin fail
		if (http.begin(client, url)) {
			code = http.GET();
			const String bodyRaw = http.getString();
			String body = bodyRaw;
			body.trim();
			Serial.printf("[GSHEET] code=%d url=%s\r\n", code, url.c_str());
			if (bodyRaw.length() > 0) {
				Serial.printf("[GSHEET] body=%s\r\n", bodyRaw.c_str());
			}

			// Parse response for name/ok.
			// Preferred: JSON {"ok":true,"name":"...","message":"..."}
			if (body.length() > 0 && body[0] == '{') {
				StaticJsonDocument<256> doc;
				if (deserializeJson(doc, body) == DeserializationError::Ok) {
					result.ok = doc["ok"] | false;
					const char* nm = doc["name"] | "";
					const char* msgTxt = doc["message"] | "";
					strncpy(result.name, nm, sizeof(result.name) - 1);
					strncpy(result.message, msgTxt, sizeof(result.message) - 1);
				}
			} else if (body.indexOf("Success") >= 0) {
				// Backward compatible with your old script that returns plain text "Success"
				result.ok = (code == 200);
				strncpy(result.message, "Success", sizeof(result.message) - 1);
			} else if (body.length() > 0) {
				// Store a short snippet for LCD/debug
				String sn = body;
				sn.replace("\r", " ");
				sn.replace("\n", " ");
				sn.trim();
				if (sn.length() > 90) sn = sn.substring(0, 90);
				strncpy(result.message, sn.c_str(), sizeof(result.message) - 1);
			}
			http.end();
		} else {
			Serial.printf("[GSHEET] begin() failed url=%s\r\n", url.c_str());
			strncpy(result.message, "begin_failed", sizeof(result.message) - 1);
		}
		client.stop();

		result.httpCode = code;
		xQueueSend(resQ, &result, 0);
	}
}

void GoogleSheetLogger::ensureTaskStarted_() {
	if (started_) return;
	started_ = true;

	uidQueue_ = xQueueCreate(6, sizeof(UidMsg));
	resultQueue_ = xQueueCreate(6, sizeof(GoogleLogResult));

	// Task on core 1. Pass `this` so the task can access queues.
	xTaskCreatePinnedToCore(GoogleSheetLogger::taskFn_, "gsheet", 6144, this, 1, nullptr, 1);
}

void GoogleSheetLogger::begin() {
	ensureTaskStarted_();
}

bool GoogleSheetLogger::enqueueEvent(const GoogleLogEvent& ev) {
	ensureTaskStarted_();
	if (ev.uid[0] == 0) return false;

	UidMsg msg;
	memset(&msg, 0, sizeof(msg));
	msg.ev = ev;

	return xQueueSend((QueueHandle_t)uidQueue_, &msg, 0) == pdTRUE;
}

bool GoogleSheetLogger::tryDequeueResult(GoogleLogResult& out) {
	ensureTaskStarted_();
	return xQueueReceive((QueueHandle_t)resultQueue_, &out, 0) == pdTRUE;
}

