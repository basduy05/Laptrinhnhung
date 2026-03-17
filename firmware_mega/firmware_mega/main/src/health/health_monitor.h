#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

class HealthMonitor {
public:
  void begin();
  void update();
  unsigned long uptime() const;
  int getFreeRAM() const;

private:
  unsigned long startTime;
};

#endif
