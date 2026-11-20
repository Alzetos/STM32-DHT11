#include "main.h"
#include "stepper_control.h"
#include "monitor_config.h"
#include <assert.h>
#include <stdio.h>
GPIO_TypeDef mock_port;
static uint16_t odr = (1U << 0) | (1U << 15);
static const uint16_t all = STEPPER_IN1_Pin | STEPPER_IN2_Pin |
                            STEPPER_IN3_Pin | STEPPER_IN4_Pin;
static void apply(void)
{
  uint32_t b = mock_port.BSRR;
  assert(((b | (b >> 16)) & 0xffffU & ~all) == 0);
  odr = (uint16_t)((odr & ~(b >> 16)) | (b & 0xffffU));
  assert((odr & ((1U << 0) | (1U << 15))) == ((1U << 0) | (1U << 15)));
}
static unsigned mask(void)
{
  return (!!(odr & STEPPER_IN1_Pin)) | (!!(odr & STEPPER_IN2_Pin) << 1) |
         (!!(odr & STEPPER_IN3_Pin) << 2) | (!!(odr & STEPPER_IN4_Pin) << 3);
}
static void tick(void) { StepperControl_Tick1ms(); apply(); }
static void ticks(unsigned n) { while (n--) tick(); }
static unsigned angle(unsigned steps)
{
  return (steps * 3600U + STEPPER_STEPS_PER_REV / 2U) / STEPPER_STEPS_PER_REV;
}
int main(void)
{
  mock_port.BSRR = 0; tick(); assert(mask() == 0); /* IRQ before Init */
  StepperControl_Init(); apply();
  assert(mask() == 0 && !StepperControl_IsMoving());
  assert(StepperControl_GetPosition10() == 0);
  for (unsigned raw = 0; raw <= 4095; ++raw) {
    StepperControl_SetTargetFromAdc(0, 0);
    StepperControl_SetTargetFromAdc((uint16_t)raw, 1);
    unsigned steps = (raw * STEPPER_STEPS_PER_REV + 2047U) / 4095U;
    assert(StepperControl_GetTarget10() == angle(steps));
  }
  StepperControl_SetTargetFromAdc(0, 1); tick();
#if STEPPER_HOLD_AT_TARGET
  assert(mask() == 1);
#else
  assert(mask() == 0);
#endif
  StepperControl_SetTargetFromAdc(4095, 1);
  /* Initial phase must be applied before the first moving step. */
  if (!mask()) tick();
  const unsigned sequence[] = {1, 3, 2, 6, 4, 12, 8, 9};
  unsigned phase = 0;
  for (unsigned step = 1; step <= STEPPER_STEPS_PER_REV; ++step) {
    unsigned before = StepperControl_GetPosition10();
    ticks(STEPPER_STEP_INTERVAL_MS - 1);
    assert(StepperControl_GetPosition10() == before);
    tick();
    phase = (phase + (STEPPER_REVERSE_DIRECTION ? 7U : 1U)) & 7U;
    assert(mask() == sequence[phase]);
    assert(StepperControl_GetPosition10() == angle(step));
  }
  assert(!StepperControl_IsMoving());
  tick();
#if STEPPER_HOLD_AT_TARGET
  assert(mask() == sequence[phase]);
#else
  assert(mask() == 0);
  ticks(20); assert(mask() == 0); /* no repeated re-energizing at rest */
#endif
  StepperControl_SetTargetFromAdc(0, 1);
  if (!mask()) tick();
  for (unsigned step = STEPPER_STEPS_PER_REV; step > 0; --step) {
    ticks(STEPPER_STEP_INTERVAL_MS);
    phase = (phase + (STEPPER_REVERSE_DIRECTION ? 1U : 7U)) & 7U;
    assert(mask() == sequence[phase]);
    assert(StepperControl_GetPosition10() == angle(step - 1U));
  }
  assert(!StepperControl_IsMoving());
  /* Noise smaller than the configured deadband keeps the previous target. */
  StepperControl_SetTargetFromAdc(1000, 1);
  unsigned saved = StepperControl_GetTarget10();
  StepperControl_SetTargetFromAdc(1001, 1);
  assert(StepperControl_GetTarget10() == saved);
  StepperControl_SetTargetFromAdc(1100, 1);
  assert(StepperControl_GetTarget10() > saved);
  ticks(21);
  StepperControl_SetTargetFromAdc(0, 0); tick();
  unsigned stopped = StepperControl_GetPosition10();
  assert(mask() == 0 && !StepperControl_IsMoving());
  ticks(100); assert(StepperControl_GetPosition10() == stopped && mask() == 0);
  StepperControl_SetTargetFromAdc(65535, 1); tick();
  assert(mask() == 0 && !StepperControl_IsMoving());
  StepperControl_SetTargetFromAdc(4095, 1); ticks(9);
  assert(StepperControl_GetPosition10() > stopped);
  StepperControl_Init(); apply();
  assert(StepperControl_GetPosition10() == 0 && mask() == 0);
  puts("PASS stepper: all ADC targets, full travel both ways, timing, phase order,");
  puts("              deadband, coil holding/release, ADC errors, other GPIOC pins preserved.");
}
