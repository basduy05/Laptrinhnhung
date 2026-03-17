#include "scheduler.h"

#define MAX_TASKS 15

static Task tasks[MAX_TASKS];
static uint8_t taskCount = 0;

void schedulerInit() {
    taskCount = 0;
}

void schedulerAdd(TaskCallback cb, unsigned long intervalMs) {
    if (taskCount < MAX_TASKS) {
        tasks[taskCount++] = {cb, intervalMs, 0};
    }
}

void schedulerRun() {
    unsigned long now = millis();
    for (uint8_t i = 0; i < taskCount; i++) {
        if (now - tasks[i].lastRun >= tasks[i].intervalMs) {
            tasks[i].lastRun = now;
            tasks[i].callback();
        }
    }
}
