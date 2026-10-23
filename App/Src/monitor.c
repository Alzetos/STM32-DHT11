#include "main.h"
#include "monitor.h"
#include "monitor_config.h"
#include "dht11.h"
#include "display.h"
#include <stdio.h>
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;
static DhtReading reading;
static DhtStatus dht_status = DHT_TIMEOUT;
static uint32_t last_dht, last_screen;
static int adc_read(uint16_t *raw) {
  uint32_t sum = 0;
  for (unsigned i = 0; i < ADC_AVERAGE_SAMPLES; ++i) {
    if (HAL_ADC_Start(&hadc1) != HAL_OK) return 0;
    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
      HAL_ADC_Stop(&hadc1); return 0;
    }
    sum += HAL_ADC_GetValue(&hadc1);
    if (HAL_ADC_Stop(&hadc1) != HAL_OK) return 0;
  }
  *raw = (uint16_t)((sum + ADC_AVERAGE_SAMPLES / 2) / ADC_AVERAGE_SAMPLES);
  return 1;
}
void Monitor_Init(void) {
  if (HAL_TIM_Base_Start(&htim2) != HAL_OK || HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) Error_Handler();
  Display_Init();
  Display_Line(0, "DHT11 + ADC / ILI9341"); Display_Line(9, "Sensor warm-up");
  HAL_Delay(2200);
  last_dht = HAL_GetTick() - DHT_INTERVAL_MS;
  last_screen = HAL_GetTick() - 250U;
}
void Monitor_Run(void) {
  uint32_t tick = HAL_GetTick();
  if ((uint32_t)(tick - last_dht) >= DHT_INTERVAL_MS) {
    dht_status = DHT11_Read(&reading);
    last_dht = HAL_GetTick();
  }
  tick = HAL_GetTick();
  if ((uint32_t)(tick - last_screen) < 250U) return;
  last_screen = tick;
  char line[40];
  if (dht_status == DHT_OK) {
    int t=reading.temperature10;
    unsigned magnitude=(unsigned)(t<0?-t:t);
    snprintf(line,sizeof line,"TEMP: %s%u.%u C",t<0?"-":"",magnitude/10,magnitude%10);
    Display_Line(2,line);
    snprintf(line,sizeof line,"HUM: %u.%u %%",reading.humidity10/10,reading.humidity10%10);
    Display_Line(3,line); Display_Line(9,"DHT11: OK");
  } else {
    Display_Line(2,"TEMP: --"); Display_Line(3,"HUM: --");
    Display_Line(9,dht_status==DHT_TIMEOUT?"DHT11: NO RESPONSE":dht_status==DHT_CHECKSUM?"DHT11: CHECKSUM":"DHT11: DATA ERROR");
  }
  uint16_t raw;
  if (adc_read(&raw)) {
    uint32_t adc_mv=((uint32_t)raw*ADC_VREF_MV+2047)/4095;
    uint64_t denominator=4095ULL*DIVIDER_BOTTOM_OHM;
    uint32_t mv=(uint32_t)(((uint64_t)raw*ADC_VREF_MV*(DIVIDER_TOP_OHM+(uint64_t)DIVIDER_BOTTOM_OHM)+denominator/2)/denominator);
    snprintf(line,sizeof line,"ADC: %u /4095",raw);Display_Line(5,line);
    snprintf(line,sizeof line,"U ADC: %lu.%03lu V",(unsigned long)(adc_mv/1000),(unsigned long)(adc_mv%1000));Display_Line(6,line);
    if(raw>=4090)snprintf(line,sizeof line,"U IN: SATURATED");
    else snprintf(line,sizeof line,"U IN: %lu.%03lu V",(unsigned long)(mv/1000),(unsigned long)(mv%1000));
    Display_Line(7,line);
  } else {Display_Line(5,"ADC ERROR");Display_Line(6,"U ADC: --");Display_Line(7,"U IN: --");}
}
