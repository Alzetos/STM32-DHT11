#ifndef STEPPER_MOCK_MAIN_H
#define STEPPER_MOCK_MAIN_H
#include <stdint.h>
static inline uint32_t __get_PRIMASK(void) { return 0; }
static inline void __disable_irq(void) { }
static inline void __set_PRIMASK(uint32_t mask) { (void)mask; }
typedef struct { volatile uint32_t BSRR; } GPIO_TypeDef;
extern GPIO_TypeDef mock_port;
#define STEPPER_GPIO_Port (&mock_port)
#define STEPPER_IN1_Pin (1U << 6)
#define STEPPER_IN2_Pin (1U << 7)
#define STEPPER_IN3_Pin (1U << 8)
#define STEPPER_IN4_Pin (1U << 9)
#endif
