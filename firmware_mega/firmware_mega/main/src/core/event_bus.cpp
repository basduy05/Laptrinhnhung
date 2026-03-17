#include "event_bus.h"

#define MAX_EVENT_HANDLER 10

struct HandlerEntry {
    EventType type;
    EventHandler handler;
};

static HandlerEntry handlers[MAX_EVENT_HANDLER];
static uint8_t handlerCount = 0;

void eventBusInit() {
    handlerCount = 0;
}

void eventBusSubscribe(EventType type, EventHandler handler) {
    if (handlerCount < MAX_EVENT_HANDLER) {
        handlers[handlerCount++] = {type, handler};
    }
}

bool eventBusPublish(const Event& evt) {
    for (uint8_t i = 0; i < handlerCount; i++) {
        if (handlers[i].type == evt.type) {
            handlers[i].handler(evt);
        }
    }
    return true;
}
