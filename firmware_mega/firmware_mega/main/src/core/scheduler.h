#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

typedef void (*TaskCallback)();

struct Task {
    TaskCallback callback;
    unsigned long intervalMs;
    unsigned long lastRun;
};

void schedulerInit();
void schedulerAdd(TaskCallback cb, unsigned long intervalMs);
void schedulerRun();

#endif
