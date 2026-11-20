#include "main.h"
#include "servo_control.h"
#include "monitor_config.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>

TIM_HandleTypeDef htim3 = {19999, 0};
static unsigned starts;
static HAL_StatusTypeDef start_status = HAL_OK;
static jmp_buf errors;
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *handle, uint32_t channel)
{
  assert(handle == &htim3 && channel == TIM_CHANNEL_1);
  assert(handle->compare == SERVO_CLOSED_US);
  ++starts;
  return start_status;
}
void Error_Handler(void) { longjmp(errors, 1); }
static void expect(ServoPosition position)
{
  assert(ServoControl_GetPosition() == position);
  unsigned pulse = position == SERVO_OPEN ? SERVO_OPEN_US : SERVO_CLOSED_US;
  assert(ServoControl_GetPulseUs() == pulse && htim3.compare == pulse);
}
int main(void)
{
  if (setjmp(errors)) assert(!"unexpected timer initialization error");
  ServoControl_Init();
  assert(starts == 1);
  expect(SERVO_CLOSED);
  ServoControl_Update(400, 1); expect(SERVO_CLOSED);
  ServoControl_Update(900, 0); expect(SERVO_CLOSED);
  ServoControl_Update(401, 1); expect(SERVO_OPEN);
  ServoControl_Update(400, 1); expect(SERVO_OPEN);
  ServoControl_Update(399, 0); expect(SERVO_OPEN);
  ServoControl_Update(1001, 1); expect(SERVO_OPEN);
  ServoControl_Update(65535, 1); expect(SERVO_OPEN);
  ServoControl_Update(399, 1); expect(SERVO_CLOSED);
  ServoControl_Update(400, 1); expect(SERVO_CLOSED);
  /* Check every physically valid humidity from each prior state. */
  for (unsigned h = 0; h <= 1000; ++h) {
    ServoControl_Update(0, 1);
    ServoControl_Update((uint16_t)h, 1);
    expect(h > 400 ? SERVO_OPEN : SERVO_CLOSED);
    ServoControl_Update(1000, 1);
    ServoControl_Update((uint16_t)h, 1);
    expect(h < 400 ? SERVO_CLOSED : SERVO_OPEN);
  }
  assert(starts == 1); /* hardware PWM remains running during all updates */
  ServoControl_Init(); expect(SERVO_CLOSED);
  start_status = HAL_ERROR;
  if (!setjmp(errors)) {
    ServoControl_Init(); assert(!"PWM startup failure must be reported");
  }
  start_status = HAL_OK;
  htim3.period = 999;
  if (!setjmp(errors)) {
    ServoControl_Init(); assert(!"LED timer period cannot drive the servo");
  }
  puts("PASS: servo startup CLOSED, all 1001 humidity values from both states,");
  puts("      equality holds, invalid readings hold, continuous PWM, timer errors.");
}
