#ifndef MONITOR_CONFIG_H
#define MONITOR_CONFIG_H
/* Select the ACTUAL installed resistors; software cannot change the divider. */
#define DIVIDER_DIRECT       0
#define DIVIDER_10K_10K      1
#define DIVIDER_30K_10K      2
#define DIVIDER_100K_10K     3
#define DIVIDER_CUSTOM       4
#ifndef DIVIDER_PROFILE
#define DIVIDER_PROFILE DIVIDER_10K_10K
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
#endif
