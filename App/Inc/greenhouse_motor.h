#ifndef GREENHOUSE_MOTOR_H
#define GREENHOUSE_MOTOR_H
#include "greenhouse_server.h"
void GreenhouseMotor_Init(void);
void GreenhouseMotor_Update(const GreenhouseServer *server, uint32_t now);
#endif
