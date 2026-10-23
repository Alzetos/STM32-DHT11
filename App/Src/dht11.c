#include "main.h"
#include "dht11.h"
extern TIM_HandleTypeDef htim2;
static uint16_t now(void) { return (uint16_t)__HAL_TIM_GET_COUNTER(&htim2); }
void DelayUs(uint16_t us) {
  uint16_t start = now();
  while ((uint16_t)(now() - start) < us) { }
}
/* Wait while the bus holds a level. All waits have a bounded timeout. */
static int pulse(GPIO_PinState level, uint16_t limit, uint16_t *width) {
  uint16_t start = now();
  while (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) == level) {
    if ((uint16_t)(now() - start) >= limit) return 0;
  }
  *width = (uint16_t)(now() - start);
  return 1;
}
DhtStatus DHT11_Read(DhtReading *reading) {
  uint8_t bytes[5] = {0};
  uint16_t width;
  /* Open drain: SET releases the bus. External pull-up is mandatory. */
  HAL_GPIO_WritePin(DHT_DATA_GPIO_Port, DHT_DATA_Pin, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(DHT_DATA_GPIO_Port, DHT_DATA_Pin, GPIO_PIN_SET);
  if (!pulse(GPIO_PIN_SET, 120, &width) ||
      !pulse(GPIO_PIN_RESET, 120, &width) || width < 50 ||
      !pulse(GPIO_PIN_SET, 120, &width) || width < 50) return DHT_TIMEOUT;
  for (unsigned bit = 0; bit < 40; ++bit) {
    if (!pulse(GPIO_PIN_RESET, 100, &width) || width < 25 ||
        !pulse(GPIO_PIN_SET, 110, &width)) return DHT_TIMEOUT;
    bytes[bit / 8] = (uint8_t)((bytes[bit / 8] << 1) | (width > 50));
  }
  if ((uint8_t)(bytes[0] + bytes[1] + bytes[2] + bytes[3]) != bytes[4]) return DHT_CHECKSUM;
  /* DHT11 decimal bytes contain tenths, unlike DHT22's 16-bit format. */
  if (bytes[0] > 100 || bytes[1] > 9 || (bytes[2] & 0x7f) > 80 ||
      bytes[3] > 9 || (bytes[0] == 100 && bytes[1] != 0)) return DHT_RANGE;
  reading->humidity10 = (uint16_t)(bytes[0] * 10 + bytes[1]);
  reading->temperature10 = (int16_t)((bytes[2] & 0x7f) * 10 + bytes[3]);
  if (bytes[2] & 0x80) reading->temperature10 = -reading->temperature10;
  return DHT_OK;
}
