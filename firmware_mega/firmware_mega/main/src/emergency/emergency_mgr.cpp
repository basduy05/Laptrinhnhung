#include "emergency_mgr.h"

void EmergencyMgr::begin() {
  emergency = false;
  dangerSinceMs = 0;
  clearSinceMs = 0;
  stage = STAGE_IDLE;
  stageStartMs = 0;
}

void EmergencyMgr::attachSensors(GasSensor* g, FireSensor* f, IRSensor* i) {
  gas = g;
  fire = f;
  ir = i;
}

void EmergencyMgr::attachActuators(FanMgr* f1, FanMgr* f2,
                                  BuzzerMgr* bz,
                                  ServoDoor* d,
                                  DoorLock* l) {
  fanA = f1;
  fanB = f2;
  buzzer = bz;
  door = d;
  lock = l;
}

void EmergencyMgr::update() {
  const unsigned long now = millis();
  const bool dangerRaw = gas->isDanger() || fire->detected();

  // Only track danger/clear timings OUTSIDE of active emergency
  // This prevents false exits from transient sensor reads
  if (!emergency) {
    if (dangerRaw) {
      if (dangerSinceMs == 0) dangerSinceMs = now;
      clearSinceMs = 0;
    } else {
      if (clearSinceMs == 0) clearSinceMs = now;
      dangerSinceMs = 0;
    }
  }

  // STAGED EMERGENCY ACTIVATION (prevent brownout)
  if (!emergency && dangerSinceMs != 0 && (now - dangerSinceMs) >= EMERGENCY_ASSERT_MS) {
    emergency = true;
    stage = STAGE_FANS;
    stageStartMs = now;
    dangerSinceMs = now;  // Lock in danger time
    clearSinceMs = 0;     // Reset clear timer
    
    // Stage 1: Buzzer only (low power)
    buzzer->alarm();
  }

  // Progress through stages with delays
  if (emergency) {
    const unsigned long stageElapsed = now - stageStartMs;
    
    switch (stage) {
      case STAGE_FANS:
        if (stageElapsed >= 200UL) {
          // Stage 2: Turn on fans ONLY if Gas is detected
          // (Do not fan the flames if it's just Fire)
          if (gas->isDanger()) {
            fanA->set(true);
            fanB->set(true);
          }
          stage = STAGE_UNLOCK;
          stageStartMs = now;
        }
        break;
        
      case STAGE_UNLOCK:
        if (stageElapsed >= 250UL) {
          // Stage 3: Unlock door
          lock->unlock();
          stage = STAGE_SERVO;
          stageStartMs = now;
        }
        break;
        
      case STAGE_SERVO:
        if (stageElapsed >= 300UL) {
          // Stage 4: Open servo (after unlock settled)
          door->open();
          stage = STAGE_ACTIVE;
        }
        break;
        
      case STAGE_ACTIVE:
        // Fully active - track danger/clear ONLY when emergency is active
        if (!dangerRaw) {
          if (clearSinceMs == 0) clearSinceMs = now;
          
          // Exit condition: Clear for 5 seconds
          if ((now - clearSinceMs) >= 5000UL) {
             // Start staged exit sequence
             stage = STAGE_EXIT_FANS;
             stageStartMs = now;
             fanA->set(false);
             fanB->set(false);
          }
        } else {
          clearSinceMs = 0;  // Reset if danger returns
        }
        break;

      case STAGE_EXIT_FANS:
        if (stageElapsed >= 500UL) {
          // Stage Exit 2: Proceed to next stage (Door remains OPEN per user request)
          stage = STAGE_EXIT_DOOR;
          stageStartMs = now;
          // door->close(); // Removed: Door stays open after emergency
        }
        break;

      case STAGE_EXIT_DOOR:
        if (stageElapsed >= 1000UL) {
          // Stage Exit 3: Proceed to next stage (Lock remains UNLOCKED)
          stage = STAGE_EXIT_LOCK;
          stageStartMs = now;
          // lock->lock(); // Removed: Lock stays unlocked
        }
        break;

      case STAGE_EXIT_LOCK:
        if (stageElapsed >= 200UL) {
          // Stage Exit 4: Finalize (Turn off buzzer last)
          stage = STAGE_EXIT_FINAL;
          stageStartMs = now;
        }
        break;

      case STAGE_EXIT_FINAL:
        if (stageElapsed >= 200UL) {
          // Finalize
          emergency = false;
          stage = STAGE_IDLE;
          dangerSinceMs = 0;
          clearSinceMs = 0;
          buzzer->off();
        }
        break;
        
      default:
        break;
    }
  }

  // Removed old monolithic exit block

}

bool EmergencyMgr::active() const {
  return emergency;
}
