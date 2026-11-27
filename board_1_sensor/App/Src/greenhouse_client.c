#include "greenhouse_client.h"
#include <string.h>
void GreenhouseClient_Init(GreenhouseClient *c, uint16_t token) {
  memset(c, 0, sizeof *c); c->hello_token = token;
}
void GreenhouseClient_Measurement(GreenhouseClient *c, int16_t t, uint16_t h, int valid) {
  LinkMessage *m = &c->measurement;
  memset(m, 0, sizeof *m);
  m->type = LINK_TYPE_MEASUREMENT; m->sequence = ++c->sequence;
  m->temperature10 = t; m->humidity10 = h; m->flags = valid ? LINK_DATA_VALID : 0;
  c->have_measurement = 1; c->pending = 1;
}
void GreenhouseClient_Request(const GreenhouseClient *c, LinkMessage *request) {
  memset(request, 0, sizeof *request);
  if (!c->ready) { request->type = LINK_TYPE_HELLO; request->sequence = c->hello_token; }
  else if (c->have_measurement && c->pending) *request = c->measurement;
  else request->type = LINK_TYPE_POLL;
}
int GreenhouseClient_Reply(GreenhouseClient *c, const uint8_t *bytes, uint32_t now) {
  LinkMessage reply;
  if (!LinkProtocol_Decode(&reply, bytes) || reply.type != LINK_TYPE_STATUS) {
    ++c->bad_replies; return 0;
  }
  c->remote = reply; c->last_reply = now; c->have_reply = 1;
  if (!c->ready) {
    if ((reply.flags & LINK_STATUS_SESSION_READY) &&
        !(reply.flags & LINK_STATUS_HAVE_MEASUREMENT) && reply.sequence == c->hello_token) {
      c->ready = 1; c->pending = c->have_measurement;
    }
  } else if (!(reply.flags & LINK_STATUS_SESSION_READY)) {
    c->ready = 0; ++c->hello_token; c->pending = c->have_measurement;
  } else if (c->have_measurement && (reply.flags & LINK_STATUS_HAVE_MEASUREMENT) &&
             reply.sequence == c->measurement.sequence) c->pending = 0;
  return 1;
}
int GreenhouseClient_Online(const GreenhouseClient *c, uint32_t now) {
  return c->have_reply && (uint32_t)(now - c->last_reply) < 1000U;
}
