#include "main.h"
#include "led_control.h"
#include "monitor_config.h"

extern TIM_HandleTypeDef htim4;
static LedControlMode current_mode;
static uint16_t duty_permille;

uint16_t LedControl_CalculateDuty(uint16_t adc_raw, LedControlMode mode)
{
  if (adc_raw < LED_ADC_THRESHOLD) return 0;
  if (mode == LED_MODE_THRESHOLD) return 1000;
  if (adc_raw > 4095U) adc_raw = 4095U;

  /* Positive duty at the threshold: >=2048 means ON in both modes. */
  const uint32_t span = 4095U - LED_ADC_THRESHOLD;
  return (uint16_t)(LED_MIN_DUTY_PERMILLE +
    ((uint32_t)(adc_raw - LED_ADC_THRESHOLD) *
      (1000U - LED_MIN_DUTY_PERMILLE) + span / 2U) / span);
}

void LedControl_Init(void)
{
  current_mode = LED_CONTROL_MODE;
  duty_permille = 0;
  /* TIM4: PSC=71, ARR=999, CH1 on PB6; one count = one permille. */
  if (__HAL_TIM_GET_AUTORELOAD(&htim4) != 999U) Error_Handler();
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);
  if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1) != HAL_OK) Error_Handler();
}

void LedControl_SetMode(LedControlMode mode)
{
  current_mode = mode == LED_MODE_THRESHOLD ? LED_MODE_THRESHOLD : LED_MODE_PWM;
  /* Clear the old duty; the next valid ADC sample applies the new mode. */
  duty_permille = 0;
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);
}

LedControlMode LedControl_GetMode(void) { return current_mode; }
uint16_t LedControl_GetDutyPermille(void) { return duty_permille; }

void LedControl_Update(uint16_t adc_raw, int adc_valid)
{
  duty_permille = adc_valid ? LedControl_CalculateDuty(adc_raw, current_mode) : 0;
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, duty_permille);
}
