#include "greenhouse_motor.h"
#include "stepper_control.h"
#include "monitor_config.h"
#if GH_STEPPER_CLOSED_STEPS >= GH_STEPPER_OPEN_STEPS || GH_STEPPER_OPEN_STEPS > STEPPER_STEPS_PER_REV
#error Greenhouse stepper endpoints must be ordered and within one calibrated revolution
#endif
void GreenhouseMotor_Init(void) { StepperControl_Init(); }
void GreenhouseMotor_Update(const GreenhouseServer *s, uint32_t now) {
  if (!s->session_ready || !s->have_measurement || !s->measurement_valid ||
      Greenhouse_Stale(&s->control, now)) {
    StepperControl_Stop(1); /* Freeze current coordinate, retaining holding torque. */
    return;
  }
  StepperControl_SetTargetSteps(s->control.open ? GH_STEPPER_OPEN_STEPS : GH_STEPPER_CLOSED_STEPS);
}
