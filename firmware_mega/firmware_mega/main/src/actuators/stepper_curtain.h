#ifndef STEPPER_CURTAIN_H
#define STEPPER_CURTAIN_H

#include <Stepper.h>

class StepperCurtain {
public:
  void begin(Stepper* stepper, int steps);
  void open();
  void close();
  void stop();
  void update();

  // -1 closing, 0 idle, 1 opening
  int motion() const;

private:
  Stepper* motor;
  int stepsPerAction;
  int pendingSteps = 0;
};

#endif
