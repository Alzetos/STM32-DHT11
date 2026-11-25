#include "greenhouse.h"
#include <string.h>
void Greenhouse_Init(Greenhouse *g) { memset(g, 0, sizeof *g); }
int Greenhouse_Stale(const Greenhouse *g, uint32_t now) {
  return !g->have_good || (uint32_t)(now - g->last_good) >= GH_DATA_TIMEOUT_MS;
}
void Greenhouse_Tick(Greenhouse *g, uint32_t now) {
  if (Greenhouse_Stale(g, now)) {
    g->confirmations = 0;
    g->candidate_valid = 0;
    /* Hold the commanded position; never silently close on communication loss. */
  }
}
void Greenhouse_Sample(Greenhouse *g, int16_t t, uint16_t h, int valid, uint32_t now) {
  Greenhouse_Tick(g, now);
  if (!valid || t < -400 || t > 800 || h > 1000U) {
    g->sensor_error = 1;
    g->confirmations = 0;
    g->candidate_valid = 0;
    return;
  }
  g->sensor_error = 0;
  g->have_good = 1;
  g->last_good = now;
  if (h > GH_HUM_OPEN10) g->humidity_request = 1;
  else if (h < GH_HUM_CLOSE10) g->humidity_request = 0;
  if (t > GH_TEMP_OPEN10) g->temperature_request = 1;
  else if (t < GH_TEMP_CLOSE10) g->temperature_request = 0;
  uint8_t desired = (uint8_t)(g->humidity_request || g->temperature_request);
  if (desired == g->open) {
    g->confirmations = 0;
    g->candidate_valid = 0;
  } else {
    if (!g->candidate_valid || g->candidate != desired) {
      g->candidate = desired;
      g->candidate_valid = 1;
      g->confirmations = 1;
    } else if (g->confirmations < GH_CONFIRM_SAMPLES) ++g->confirmations;
    if (g->confirmations >= GH_CONFIRM_SAMPLES) {
      g->open = desired;
      g->confirmations = 0;
      g->candidate_valid = 0;
    }
  }
}
uint8_t Greenhouse_Reasons(const Greenhouse *g) {
  return (uint8_t)((g->humidity_request ? GH_REASON_HUMIDITY : 0) |
                   (g->temperature_request ? GH_REASON_TEMPERATURE : 0));
}
