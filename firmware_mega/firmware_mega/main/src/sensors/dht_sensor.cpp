#include "dht_sensor.h"

void DHTSensor::begin(uint8_t pin, uint8_t type) {
  if (!initialized) {
    dht = new (dhtStorage) DHT(pin, type);
    initialized = true;
  }
  if (dht) dht->begin();
}

void DHTSensor::update() {
  if (!dht) return;
  if (millis() - lastRead < 2000) return;
  float nt = dht->readTemperature();
  float nh = dht->readHumidity();
  if (!isnan(nt)) t = nt;
  if (!isnan(nh)) h = nh;
  lastRead = millis();
}

float DHTSensor::temperature() const { return t; }
float DHTSensor::humidity() const { return h; }
