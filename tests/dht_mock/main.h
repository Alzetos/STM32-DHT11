#ifndef DHT_MOCK_MAIN_H
#define DHT_MOCK_MAIN_H
#include <stdint.h>
typedef int TIM_HandleTypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
#define DHT_DATA_GPIO_Port 0
#define DHT_DATA_Pin 8
uint16_t fake_counter(void);
#define __HAL_TIM_GET_COUNTER(handle) fake_counter()
GPIO_PinState HAL_GPIO_ReadPin(int port, int pin);
void HAL_GPIO_WritePin(int port, int pin, GPIO_PinState state);
void HAL_Delay(uint32_t ms);
#endif
