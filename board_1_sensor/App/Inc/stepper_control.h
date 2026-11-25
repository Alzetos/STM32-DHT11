#ifndef STEPPER_CONTROL_H
#define STEPPER_CONTROL_H
#include <stdint.h>
void StepperControl_Init(void);
void StepperControl_SetTargetFromAdc(uint16_t raw, int valid);
/* Called once per SysTick (1 ms), including while main is blocked. */
void StepperControl_Tick1ms(void);
uint16_t StepperControl_GetPosition10(void);
uint16_t StepperControl_GetTarget10(void);
int StepperControl_IsMoving(void);
#endif
