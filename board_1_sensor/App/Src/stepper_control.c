#include "main.h"
#include "stepper_control.h"
#include "monitor_config.h"

/* IN1,IN2,IN3,IN4 half-step sequence. Only these GPIO bits are touched. */
static const uint8_t sequence[8] = {1, 3, 2, 6, 4, 12, 8, 9};
static volatile uint32_t position, target;
static volatile uint8_t initialized, enabled;
static uint8_t phase, elapsed, applied;

static void drive(uint8_t mask)
{
  if (mask == applied) return;
  uint16_t pins = 0;
  if (mask & 1U) pins |= STEPPER_IN1_Pin;
  if (mask & 2U) pins |= STEPPER_IN2_Pin;
  if (mask & 4U) pins |= STEPPER_IN3_Pin;
  if (mask & 8U) pins |= STEPPER_IN4_Pin;
  const uint16_t all = STEPPER_IN1_Pin | STEPPER_IN2_Pin |
                       STEPPER_IN3_Pin | STEPPER_IN4_Pin;
  /* Atomic set/reset preserves every other GPIOC output. */
  STEPPER_GPIO_Port->BSRR = ((uint32_t)(all & ~pins) << 16) | pins;
  applied = mask;
}

void StepperControl_Init(void)
{
  initialized = 0;
  enabled = 0;
  position = target = 0; /* present shaft position is the software zero */
  phase = elapsed = 0;
  applied = 255;
  drive(0);
  initialized = 1;
}

void StepperControl_SetTargetFromAdc(uint16_t raw, int valid)
{
  if (!valid || raw > 4095U) {
    enabled = 0; /* next tick releases coils; no stale ADC command */
    return;
  }
  const uint32_t next = ((uint32_t)raw * STEPPER_STEPS_PER_REV + 2047U) / 4095U;
  const uint32_t old = target;
  const uint32_t delta = next > old ? next - old : old - next;
  if (!enabled || next == 0 || next == STEPPER_STEPS_PER_REV ||
      delta >= STEPPER_TARGET_DEADBAND_STEPS) target = next;
  enabled = 1;
}

void StepperControl_Tick1ms(void)
{
  if (!initialized) return;
  if (!enabled) { drive(0); elapsed = 0; return; }
  const uint32_t goal = target;
  const uint32_t current = position;
  if (current == goal) {
    elapsed = 0;
#if STEPPER_HOLD_AT_TARGET
    drive(sequence[phase]);
#else
    drive(0);
#endif
    return;
  }
  /* Energize the current phase, then wait before the next half-step. */
  if (applied == 0) { drive(sequence[phase]); elapsed = 0; return; }
  if (++elapsed < STEPPER_STEP_INTERVAL_MS) return;
  elapsed = 0;
  const int forward = current < goal;
  const int electrical_forward = forward != STEPPER_REVERSE_DIRECTION;
  phase = (uint8_t)((phase + (electrical_forward ? 1U : 7U)) & 7U);
  drive(sequence[phase]);
  position = forward ? current + 1U : current - 1U;
}

static uint16_t angle10(uint32_t steps)
{
  return (uint16_t)((steps * 3600U + STEPPER_STEPS_PER_REV / 2U) /
                    STEPPER_STEPS_PER_REV);
}
uint16_t StepperControl_GetPosition10(void) { return angle10(position); }
uint16_t StepperControl_GetTarget10(void) { return angle10(target); }
int StepperControl_IsMoving(void) { return enabled && position != target; }
