#ifndef GREENHOUSE_CLIENT_H
#define GREENHOUSE_CLIENT_H
#include "spi_protocol.h"
typedef struct {
  SpiMessage measurement, remote;
  uint16_t sequence, hello_token;
  uint8_t have_measurement, have_reply, ready, pending;
  uint32_t last_reply, bad_replies;
} GreenhouseClient;
void GreenhouseClient_Init(GreenhouseClient *c, uint16_t token);
void GreenhouseClient_Measurement(GreenhouseClient *c, int16_t t, uint16_t h, int valid);
void GreenhouseClient_Request(const GreenhouseClient *c, SpiMessage *request);
int GreenhouseClient_Reply(GreenhouseClient *c, const uint8_t *bytes, uint32_t now);
int GreenhouseClient_Online(const GreenhouseClient *c, uint32_t now);
#endif
