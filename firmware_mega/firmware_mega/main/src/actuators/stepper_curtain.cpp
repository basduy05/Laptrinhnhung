#include "stepper_curtain.h"

void StepperCurtain::begin(Stepper* s, int steps) {
  motor = s;
  stepsPerAction = steps;
}

void StepperCurtain::open() {
  if (pendingSteps > 0) {
    pendingSteps = 0;
    return;
  }
  pendingSteps = stepsPerAction;
}

void StepperCurtain::close() {
  if (pendingSteps < 0) {
    pendingSteps = 0;
    return;
  }
  pendingSteps = -stepsPerAction;
}

void StepperCurtain::stop() {
  pendingSteps = 0;
}

void StepperCurtain::update() {
  if (pendingSteps != 0) {
    motor->step(pendingSteps > 0 ? 1 : -1);
    pendingSteps += (pendingSteps > 0 ? -1 : 1);
  }
}

int StepperCurtain::motion() const {
  if (pendingSteps > 0) return 1;
  if (pendingSteps < 0) return -1;
  return 0;
}
