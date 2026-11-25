#ifndef GREENHOUSE_SERVER_H
#define GREENHOUSE_SERVER_H
#include "greenhouse.h"
#include "spi_protocol.h"
typedef struct {
  Greenhouse control;
  uint16_t sequence, hello_token;
  uint8_t session_ready, have_measurement, measurement_valid, rx_error;
} GreenhouseServer;
void GreenhouseServer_Init(GreenhouseServer *s);
/* Receives a validated decoded packet. Returns 1 only for a new measurement. */
int GreenhouseServer_Receive(GreenhouseServer *s, const SpiMessage *m, uint32_t now);
void GreenhouseServer_Status(GreenhouseServer *s, uint32_t now, uint16_t pulse_us,
                             SpiMessage *out);
#endif
