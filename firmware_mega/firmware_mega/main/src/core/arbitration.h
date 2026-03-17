#ifndef ARBITRATION_H
#define ARBITRATION_H

#include "system_state.h"
#include "event_bus.h"

/**
 * Decide whether an action is allowed
 * based on current system priority
 */
bool arbitrationAllow(EventType evt);

/**
 * Update system priority
 */
void arbitrationSetPriority(PriorityLevel level);

/**
 * Get current priority
 */
PriorityLevel arbitrationGetPriority();

#endif
