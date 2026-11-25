#include "main.h"
#include "servo_control.h"
#include "monitor_config.h"

extern TIM_HandleTypeDef htim3;
static ServoPosition position;
static uint16_t pulse_us;

static void set_position(ServoPosition next)
{
  position = next;
  pulse_us = next == SERVO_OPEN ? SERVO_OPEN_US : SERVO_CLOSED_US;
  /* TIM3 runs at 1 MHz: CCR1 directly sets the HIGH duration in us. */
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse_us);
}

void ServoControl_Init(void)
{
  if (__HAL_TIM_GET_AUTORELOAD(&htim3) != SERVO_PERIOD_US - 1U)
    Error_Handler();
  set_position(SERVO_CLOSED);
  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
    Error_Handler();
}

void ServoControl_Update(uint16_t humidity10, int reading_valid)
{
  /* Invalid/missing data and exactly 40% keep the previous command. */
  if (!reading_valid || humidity10 > 1000U) return;
  if (humidity10 < SERVO_HUMIDITY_THRESHOLD10) set_position(SERVO_CLOSED);
  else if (humidity10 > SERVO_HUMIDITY_THRESHOLD10) set_position(SERVO_OPEN);
}

void ServoControl_SetPosition(ServoPosition next)
{
  if (next == SERVO_CLOSED || next == SERVO_OPEN) set_position(next);
}

ServoPosition ServoControl_GetPosition(void) { return position; }
uint16_t ServoControl_GetPulseUs(void) { return pulse_us; }
