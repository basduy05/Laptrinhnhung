#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include <DHT.h>
#include <new>

class DHTSensor {
public:
  void begin(uint8_t pin, uint8_t type);
  void update();
  float temperature() const;
  float humidity() const;

private:
  alignas(DHT) uint8_t dhtStorage[sizeof(DHT)];
  DHT* dht = nullptr;
  bool initialized = false;
  float t = 0, h = 0;
  unsigned long lastRead = 0;
};

#endif
