#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <Arduino.h>

enum EventType : uint8_t {
    EVT_NONE = 0,
    EVT_SYSTEM_TICK,
    EVT_KEYPAD_INPUT,

    EVT_CMD_OPEN_DOOR,
    EVT_CMD_CLOSE_DOOR,
    EVT_CMD_LOCK,
    EVT_CMD_UNLOCK,
    EVT_CMD_CURTAIN_OPEN,
    EVT_CMD_CURTAIN_CLOSE,
    EVT_CMD_CURTAIN_STOP,

    EVT_CMD_FAN1_ON,
    EVT_CMD_FAN1_OFF,
    EVT_CMD_FAN2_ON,
    EVT_CMD_FAN2_OFF
};

struct Event {
    EventType type = EVT_NONE;
    int32_t value = 0;
};

typedef void (*EventHandler)(const Event& evt);

void eventBusInit();
void eventBusSubscribe(EventType type, EventHandler handler);
bool eventBusPublish(const Event& evt);

#endif // EVENT_BUS_H
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
};

extern SystemState g_systemState;

#endif
