#ifndef MONITOR_CONFIG_H
#define MONITOR_CONFIG_H
/* Select the ACTUAL installed resistors; software cannot change the divider. */
#define DIVIDER_DIRECT       0
#define DIVIDER_10K_10K      1
#define DIVIDER_30K_10K      2
#define DIVIDER_100K_10K     3
#define DIVIDER_CUSTOM       4
#ifndef DIVIDER_PROFILE
/* Current wiring: on-board P1 wiper through J8 (PA4) to ADC1_IN4. */
#define DIVIDER_PROFILE DIVIDER_DIRECT
#endif
#if DIVIDER_PROFILE == DIVIDER_DIRECT
#define DIVIDER_TOP_OHM 0UL
#define DIVIDER_BOTTOM_OHM 10000UL
#elif DIVIDER_PROFILE == DIVIDER_10K_10K
#define DIVIDER_TOP_OHM 10000UL
#define DIVIDER_BOTTOM_OHM 10000UL
#elif DIVIDER_PROFILE == DIVIDER_30K_10K
#define DIVIDER_TOP_OHM 30000UL
#define DIVIDER_BOTTOM_OHM 10000UL
#elif DIVIDER_PROFILE == DIVIDER_100K_10K
#define DIVIDER_TOP_OHM 100000UL
#define DIVIDER_BOTTOM_OHM 10000UL
#elif DIVIDER_PROFILE == DIVIDER_CUSTOM
#define DIVIDER_TOP_OHM 47000UL
#define DIVIDER_BOTTOM_OHM 10000UL
#else
#error Unknown DIVIDER_PROFILE
#endif
#if DIVIDER_BOTTOM_OHM == 0
#error Bottom resistor must be nonzero
#endif
/* Replace with the measured voltage between VREF+ and VSSA, in millivolts. */
#define ADC_VREF_MV 3300UL
#define ADC_AVERAGE_SAMPLES 16U
#define DHT_INTERVAL_MS 2200U

/* PB6 / TIM4_CH1, active HIGH. Threshold mode: 0 or 100% duty.
 * PWM mode: OFF below 2048, then 1%..100% over ADC 2048..4095. */
#define LED_CONTROL_MODE LED_MODE_PWM
#define LED_ADC_THRESHOLD 2048U
#define LED_MIN_DUTY_PERMILLE 10U
#define ADC_UPDATE_MS 20U
#if LED_ADC_THRESHOLD == 0 || LED_ADC_THRESHOLD >= 4095
#error LED_ADC_THRESHOLD must be between 1 and 4094
#endif
#if LED_MIN_DUTY_PERMILLE == 0 || LED_MIN_DUTY_PERMILLE > 1000
#error LED_MIN_DUTY_PERMILLE must be between 1 and 1000
#endif
#endif
