#include "main.h"
#include "led_control.h"
#include "monitor_config.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>

TIM_HandleTypeDef htim4 = {999, 123};
static unsigned starts;
static HAL_StatusTypeDef start_status = HAL_OK;
static jmp_buf error_context;

HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *handle, uint32_t channel)
{
  assert(handle == &htim4 && channel == TIM_CHANNEL_1);
  assert(handle->compare == 0); /* never flash ON during PWM start */
  starts++;
  return start_status;
}
void Error_Handler(void) { longjmp(error_context, 1); }

int main(void)
{
  if (setjmp(error_context)) { assert(!"unexpected Error_Handler"); }
  LedControl_Init();
  assert(starts == 1 && htim4.compare == 0);
  assert(LedControl_GetMode() == LED_CONTROL_MODE);
  LedControl_SetMode(LED_MODE_PWM);

  unsigned previous = 0;
  for (unsigned raw = 0; raw <= 4095; raw++) {
    unsigned pwm = LedControl_CalculateDuty((uint16_t)raw, LED_MODE_PWM);
    unsigned onoff = LedControl_CalculateDuty((uint16_t)raw, LED_MODE_THRESHOLD);
    assert(pwm >= previous && pwm <= 1000);
    assert((pwm > 0) == (raw >= 2048));
    assert(onoff == (raw < 2048 ? 0U : 1000U));
    LedControl_Update((uint16_t)raw, 1);
    assert(htim4.compare == pwm && LedControl_GetDutyPermille() == pwm);
    previous = pwm;
  }
  assert(LedControl_CalculateDuty(2047, LED_MODE_PWM) == 0);
  assert(LedControl_CalculateDuty(2048, LED_MODE_PWM) == 10);
  assert(LedControl_CalculateDuty(3072, LED_MODE_PWM) == 505);
  assert(LedControl_CalculateDuty(4095, LED_MODE_PWM) == 1000);
  assert(LedControl_CalculateDuty(65535, LED_MODE_PWM) == 1000);
  LedControl_Update(4095, 0);
  assert(htim4.compare == 0 && LedControl_GetDutyPermille() == 0);

  LedControl_SetMode(LED_MODE_THRESHOLD);
  assert(htim4.compare == 0 && LedControl_GetMode() == LED_MODE_THRESHOLD);
  LedControl_Update(2048, 1); assert(htim4.compare == 1000);
  LedControl_Update(2047, 1); assert(htim4.compare == 0);
  LedControl_Update(4095, 0); assert(htim4.compare == 0);
  LedControl_SetMode(LED_MODE_PWM);
  LedControl_Update(2048, 1); assert(htim4.compare == 10);

  start_status = HAL_ERROR;
  if (!setjmp(error_context)) {
    LedControl_Init();
    assert(!"failed PWM start must call Error_Handler");
  }
  start_status = HAL_OK;
  htim4.period = 998;
  if (!setjmp(error_context)) {
    LedControl_Init();
    assert(!"wrong PWM period must call Error_Handler");
  }
  puts("PASS: all 4096 ADC codes, monotonic PWM, threshold boundary, full ON/OFF,");
  puts("      mode switching, ADC failure, PWM startup and wrong timer period.");
}
