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

/* 28BYJ-48 + ULN2003: IN1=PC6, IN2=PC7, IN3=PC8, IN4=PC9.
 * Nominal half-steps per output revolution; calibrate the actual gearbox. */
#define STEPPER_STEPS_PER_REV 4096U
#define STEPPER_STEP_INTERVAL_MS 4U
#define STEPPER_TARGET_DEADBAND_STEPS 8U
#ifndef STEPPER_REVERSE_DIRECTION
#define STEPPER_REVERSE_DIRECTION 0
#endif
#ifndef STEPPER_HOLD_AT_TARGET
#define STEPPER_HOLD_AT_TARGET 1
#endif
#if STEPPER_STEPS_PER_REV == 0 || STEPPER_STEPS_PER_REV > 65535
#error Invalid stepper revolution calibration
#endif
#if STEPPER_STEP_INTERVAL_MS < 2 || STEPPER_STEP_INTERVAL_MS > 255
#error Step interval must be within 2..255 ms
#endif
#if STEPPER_TARGET_DEADBAND_STEPS == 0 || STEPPER_TARGET_DEADBAND_STEPS > STEPPER_STEPS_PER_REV
#error Invalid stepper target deadband
#endif
#if (STEPPER_REVERSE_DIRECTION != 0 && STEPPER_REVERSE_DIRECTION != 1) || (STEPPER_HOLD_AT_TARGET != 0 && STEPPER_HOLD_AT_TARGET != 1)
#error Stepper direction and holding flags must be 0 or 1
#endif

/* SG90 signal: PA6 / TIM3_CH1, 50 Hz, one timer count = 1 us.
 * Calibrate both commands for the actual horn/linkage before attaching it.
 * Start with 1000 us CLOSED and 1500 us OPEN (no claimed physical angles). */
#define SERVO_PERIOD_US 20000U
#define SERVO_HUMIDITY_THRESHOLD10 400U
#define SERVO_CLOSED_US 1000U
#define SERVO_OPEN_US 1500U
#if SERVO_PERIOD_US != 20000U
#error TIM3 IOC and HAL initialization require a 20000 us period
#endif
#if SERVO_HUMIDITY_THRESHOLD10 > 1000U
#error SERVO_HUMIDITY_THRESHOLD10 must be within 0..1000
#endif
#if SERVO_CLOSED_US < 1000U || SERVO_CLOSED_US > 2000U || SERVO_OPEN_US < 1000U || SERVO_OPEN_US > 2000U
#error Servo pulse commands must be within the initial 1000..2000 us range
#endif
#if SERVO_CLOSED_US == SERVO_OPEN_US
#error Servo OPEN and CLOSED commands must differ
#endif
#if LED_ADC_THRESHOLD == 0 || LED_ADC_THRESHOLD >= 4095
#error LED_ADC_THRESHOLD must be between 1 and 4094
#endif
#if LED_MIN_DUTY_PERMILLE == 0 || LED_MIN_DUTY_PERMILLE > 1000
#error LED_MIN_DUTY_PERMILLE must be between 1 and 1000
#endif
#endif
