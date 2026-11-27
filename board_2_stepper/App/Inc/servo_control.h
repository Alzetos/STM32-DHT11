#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H
#include <stdint.h>
/* Commanded position; the three-wire servo provides no position feedback. */
typedef enum { SERVO_CLOSED, SERVO_OPEN } ServoPosition;
void ServoControl_Init(void);
void ServoControl_Update(uint16_t humidity10, int reading_valid);
void ServoControl_SetPosition(ServoPosition next);
ServoPosition ServoControl_GetPosition(void);
uint16_t ServoControl_GetPulseUs(void);
#endif
