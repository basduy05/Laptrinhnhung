#ifndef EMERGENCY_MGR_H
#define EMERGENCY_MGR_H

#include "../sensors/gas_sensor.h"
#include "../sensors/fire_sensor.h"
#include "../sensors/ir_sensor.h"

#include "../actuators/fan_mgr.h"
#include "../actuators/buzzer_mgr.h"
#include "../actuators/servo_door.h"
#include "../actuators/door_lock.h"

#include "../core/config.h"

class EmergencyMgr {
public:
  void begin();
  void update();

  bool active() const;

  void attachSensors(GasSensor* g, FireSensor* f, IRSensor* ir);
  void attachActuators(FanMgr* fan1, FanMgr* fan2,
                       BuzzerMgr* buzzer,
                       ServoDoor* door,
                       DoorLock* lock);

private:
  GasSensor* gas;
  FireSensor* fire;
  IRSensor* ir;

  FanMgr* fanA;
  FanMgr* fanB;
  BuzzerMgr* buzzer;
  ServoDoor* door;
  DoorLock* lock;

  bool emergency = false;

  unsigned long dangerSinceMs = 0;
  unsigned long clearSinceMs = 0;

  enum EmergencyStage { 
    STAGE_IDLE, 
    STAGE_FANS, 
    STAGE_UNLOCK, 
    STAGE_SERVO, 
    STAGE_ACTIVE,
    // Deactivation stages
    STAGE_EXIT_FANS,
    STAGE_EXIT_DOOR,
    STAGE_EXIT_LOCK,
    STAGE_EXIT_FINAL
  };
  EmergencyStage stage = STAGE_IDLE;
  unsigned long stageStartMs = 0;
};

#endif
