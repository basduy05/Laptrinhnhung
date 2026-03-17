#include "wifi_mgr.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <time.h>

#include "../core/config.h"

static bool g_apStarted = false;
static unsigned long g_lastAttemptMs = 0;
static bool g_mdnsStarted = false;

static void ensureMdns_() {
	if (g_mdnsStarted) return;
	if (WiFi.status() != WL_CONNECTED) return;
	// Note: .local mDNS on Windows may require Bonjour or compatible resolver.
	if (MDNS.begin("miki")) {
		MDNS.addService("http", "tcp", 80);
		g_mdnsStarted = true;
		Serial.println("[WIFI] mDNS: http://miki.local/");
	}
}

static void wifiCommonConfig_() {
	WiFi.persistent(false);
	WiFi.setAutoReconnect(true);
	WiFi.setSleep(false);
}

bool wifiConnected() {
	return WiFi.status() == WL_CONNECTED;
}

bool connectWiFi(uint32_t timeoutMs) {
	wifiCommonConfig_();
	WiFi.setHostname("miki");

	if (WiFi.status() == WL_CONNECTED) {
		// Keep SoftAP if it was started as fallback (stable web access).
		return true;
	}

	// Prefer STA (phone hotspot). If AP fallback already started, keep AP+STA.
	const wifi_mode_t wantMode = g_apStarted ? WIFI_AP_STA : WIFI_STA;
	if (WiFi.getMode() != wantMode) {
		WiFi.mode(wantMode);
	}

	// Attempt reconnect. Avoid tearing down AP if it's active.
	WiFi.begin(WIFI_SSID, WIFI_PASS);

	const uint32_t startMs = millis();
	while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < timeoutMs) {
		delay(250);
	}

	if (WiFi.status() == WL_CONNECTED) {
		Serial.print("[WIFI] connected, IP: ");
		Serial.println(WiFi.localIP());
		if (g_apStarted) {
			Serial.print("[WIFI] AP still ON, AP IP: ");
			Serial.println(WiFi.softAPIP());
		}
		ensureMdns_();

		// Time sync (Vietnam UTC+7). Use TZ to avoid manual offsets.
		setenv("TZ", "UTC-7", 1);
		tzset();
		configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");

		return true;
	}

	g_mdnsStarted = false;

	if (!g_apStarted) {
		Serial.println("[WIFI] connect timeout -> starting SoftAP fallback");
		WiFi.mode(WIFI_AP_STA);
		WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
		g_apStarted = true;
		Serial.print("[WIFI] AP IP: ");
		Serial.println(WiFi.softAPIP());
	}
	return false;
}

void wifiLoop() {
	// Lightweight reconnect loop, throttled.
	const unsigned long now = millis();
	if (wifiConnected()) return;
	if (now - g_lastAttemptMs < 5000UL) return;
	g_lastAttemptMs = now;
	// Keep AP fallback stable if it is active; do short reconnect attempts.
	(void)connectWiFi(800);
}
