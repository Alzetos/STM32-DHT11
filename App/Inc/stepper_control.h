#ifndef STEPPER_CONTROL_H
#define STEPPER_CONTROL_H
#include <stdint.h>
void StepperControl_Init(void);
void StepperControl_SetTargetFromAdc(uint16_t raw, int valid);
/* Absolute software coordinate, in half-steps since power-on. */
int StepperControl_SetTargetSteps(uint16_t steps);
/* Stop at the current coordinate; hold=1 keeps the last phase energized. */
void StepperControl_Stop(int hold);
uint16_t StepperControl_GetPositionSteps(void);
uint16_t StepperControl_GetTargetSteps(void);
/* Called once per SysTick (1 ms), including while main is blocked. */
void StepperControl_Tick1ms(void);
uint16_t StepperControl_GetPosition10(void);
uint16_t StepperControl_GetTarget10(void);
int StepperControl_IsMoving(void);
#endif
