#include "greenhouse_server.h"
#include <string.h>
void GreenhouseServer_Init(GreenhouseServer *s) { memset(s, 0, sizeof *s); }
int GreenhouseServer_Receive(GreenhouseServer *s, const SpiMessage *m, uint32_t now) {
  if (m->type == SPI_TYPE_HELLO) {
    s->hello_token = m->sequence;
    s->session_ready = 1;
    s->have_measurement = 0;
    s->measurement_valid = 0;
    s->rx_error = 0;
    s->control.have_good = 0;
    s->control.sensor_error = 0;
    s->control.confirmations = 0;
    s->control.candidate_valid = 0;
    return 0;
  }
  if (m->type != SPI_TYPE_MEASUREMENT || !s->session_ready) return 0;
  if (s->have_measurement && !SpiProtocol_IsNewSequence(m->sequence, s->sequence)) return 0;
  s->sequence = m->sequence;
  s->have_measurement = 1;
  s->measurement_valid = !!(m->flags & SPI_DATA_VALID);
  s->rx_error = 0;
  Greenhouse_Sample(&s->control, m->temperature10, m->humidity10,
                     s->measurement_valid, now);
  return 1;
}
void GreenhouseServer_Status(GreenhouseServer *s, uint32_t now, uint16_t pulse,
                             SpiMessage *out) {
  Greenhouse_Tick(&s->control, now);
  memset(out, 0, sizeof *out);
  out->type = SPI_TYPE_STATUS;
  out->sequence = s->have_measurement ? s->sequence : s->hello_token;
  out->pulse_us = pulse;
  uint32_t age = s->control.have_good ? (uint32_t)(now - s->control.last_good) / 1000U : 65535U;
  out->age_seconds = (uint16_t)(age > 65535U ? 65535U : age);
  out->flags = (uint8_t)((s->session_ready ? SPI_STATUS_SESSION_READY : 0) |
    (s->have_measurement ? SPI_STATUS_HAVE_MEASUREMENT : 0) |
    (s->measurement_valid && !Greenhouse_Stale(&s->control, now) ? SPI_STATUS_DATA_VALID : 0) |
    (Greenhouse_Stale(&s->control, now) ? SPI_STATUS_STALE : 0) |
    (s->control.sensor_error ? SPI_STATUS_SENSOR_ERROR : 0) |
    (s->rx_error ? SPI_STATUS_RX_ERROR : 0) |
    (s->control.open ? SPI_STATUS_OPEN : 0));
  out->detail = Greenhouse_Reasons(&s->control);
}
