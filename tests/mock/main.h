#ifndef MOCK_MAIN_H
#define MOCK_MAIN_H
#include <stdint.h>
typedef struct { uint32_t period, compare; } TIM_HandleTypeDef;
typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
#define TIM_CHANNEL_1 1U
#define __HAL_TIM_GET_AUTORELOAD(handle) ((handle)->period)
#define __HAL_TIM_SET_COMPARE(handle, channel, value) ((handle)->compare = (value))
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *handle, uint32_t channel);
void Error_Handler(void);
#endif
