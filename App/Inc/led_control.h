#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdint.h>

typedef enum { LED_MODE_THRESHOLD, LED_MODE_PWM } LedControlMode;

void LedControl_Init(void);
void LedControl_SetMode(LedControlMode mode);
LedControlMode LedControl_GetMode(void);
void LedControl_Update(uint16_t adc_raw, int adc_valid);
uint16_t LedControl_GetDutyPermille(void);
uint16_t LedControl_CalculateDuty(uint16_t adc_raw, LedControlMode mode);

#endif
