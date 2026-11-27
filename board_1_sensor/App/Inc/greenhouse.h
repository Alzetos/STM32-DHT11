#ifndef GREENHOUSE_H
#define GREENHOUSE_H
#include <stdint.h>
/* Teaching setpoints, not crop-specific agronomic recommendations. */
#define GH_HUM_OPEN10 400U
#define GH_HUM_CLOSE10 380U
#define GH_TEMP_OPEN10 280
#define GH_TEMP_CLOSE10 260
#define GH_CONFIRM_SAMPLES 2U
#define GH_DATA_TIMEOUT_MS 10000U
/* Nominal 90 degrees for the current 4096-half-step gearbox calibration.
 * No homing sensor: put the mechanism CLOSED before powering the board. */
#define GH_STEPPER_CLOSED_STEPS 0U
#define GH_STEPPER_OPEN_STEPS 1024U
enum { GH_REASON_HUMIDITY = 1, GH_REASON_TEMPERATURE = 2 };
typedef struct {
  uint8_t open, humidity_request, temperature_request, confirmations;
  uint8_t candidate, candidate_valid, have_good, sensor_error;
  uint32_t last_good;
} Greenhouse;
void Greenhouse_Init(Greenhouse *g);
/* Call once per NEW measurement, including failed DHT11 attempts. */
void Greenhouse_Sample(Greenhouse *g, int16_t t10, uint16_t h10,
                       int valid, uint32_t now);
void Greenhouse_Tick(Greenhouse *g, uint32_t now);
int Greenhouse_Stale(const Greenhouse *g, uint32_t now);
uint8_t Greenhouse_Reasons(const Greenhouse *g);
#endif
