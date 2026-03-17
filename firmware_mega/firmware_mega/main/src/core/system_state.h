#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <Arduino.h>
#include "config.h"

/*************************************************
 * ACTUATOR STATE
 *************************************************/

enum DoorState {
    DOOR_CLOSED = 0,
    DOOR_OPENING,
    DOOR_OPEN,
    DOOR_CLOSING,
    DOOR_LOCKED
};

enum CurtainState {
    CURTAIN_CLOSED = 0,
    CURTAIN_OPENING,
    CURTAIN_OPEN,
    CURTAIN_CLOSING
};

enum RelayState {
    RELAY_OFF = 0,
    RELAY_ON
};

enum AlarmState {
    ALARM_IDLE = 0,
    ALARM_ACTIVE
};

/*************************************************
 * SYSTEM GLOBAL STATE
 *************************************************/

struct SystemState {
    // Mode & priority
    SystemMode mode;
    PriorityLevel priority;

    // Actuators
    DoorState door;
    CurtainState curtain;
    RelayState light;
    RelayState fan;
    AlarmState alarm;

    // Sensor flags
    bool gasDetected;
    bool fireDetected;
    bool intrusionDetected;

    // Security
    bool authenticated;
    uint8_t passwordRetry;

    // Health
    unsigned long uptimeMs;
    uint16_t freeRam;

    void init() {
        mode = MODE_AUTO;
        priority = PRIORITY_AUTO;

        door = DOOR_CLOSED;
        curtain = CURTAIN_CLOSED;
        light = RELAY_OFF;
        fan = RELAY_OFF;
        alarm = ALARM_IDLE;

        gasDetected = false;
        fireDetected = false;
        intrusionDetected = false;

        authenticated = false;
        passwordRetry = 0;

        uptimeMs = 0;
        freeRam = 0;
    }

    void setMode(SystemMode m) { mode = m; }
    SystemMode getMode() const { return mode; }
};

extern SystemState g_systemState;

#endif // SYSTEM_STATE_H
