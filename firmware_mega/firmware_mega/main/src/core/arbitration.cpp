#include "arbitration.h"

static PriorityLevel currentPriority = PRIORITY_AUTO;

bool arbitrationAllow(EventType evt) {
    // Emergency always allowed
    if (currentPriority == PRIORITY_EMERGENCY) {
        return true;
    }

    // Manual allowed unless emergency
    if (currentPriority == PRIORITY_MANUAL) {
        return evt != EVT_SYSTEM_TICK;
    }

    // Auto mode: block dangerous manual commands
    if (currentPriority == PRIORITY_AUTO) {
        if (evt == EVT_CMD_OPEN_DOOR ||
            evt == EVT_CMD_CLOSE_DOOR) {
            return false;
        }
    }

    return true;
}

void arbitrationSetPriority(PriorityLevel level) {
    currentPriority = level;
    g_systemState.priority = level;
}

PriorityLevel arbitrationGetPriority() {
    return currentPriority;
}
