#ifndef LINK_PROTOCOL_H
#define LINK_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>
#define LINK_PACKET_SIZE 14U
#define LINK_PROTOCOL_VERSION 2U
#define LINK_TYPE_MEASUREMENT 0x10U
#define LINK_TYPE_POLL 0x20U
#define LINK_TYPE_HELLO 0x30U
#define LINK_TYPE_STATUS 0x90U
#define LINK_DATA_VALID 0x01U
enum {
  LINK_STATUS_HAVE_MEASUREMENT = 1U, LINK_STATUS_DATA_VALID = 2U,
  LINK_STATUS_STALE = 4U, LINK_STATUS_SENSOR_ERROR = 8U,
  LINK_STATUS_RX_ERROR = 16U, LINK_STATUS_OPEN = 32U,
  LINK_STATUS_SESSION_READY = 64U, LINK_STATUS_MOVING = 128U
};
enum {
  LINK_DETAIL_HUMIDITY = 1U, LINK_DETAIL_TEMPERATURE = 2U,
  LINK_DETAIL_TARGET_OPEN = 4U, LINK_DETAIL_AT_CLOSED = 8U
};
typedef struct {
  uint8_t type, flags, detail;
  uint16_t sequence;
  int16_t temperature10;
  uint16_t humidity10;
  uint16_t position_steps, age_seconds; /* STATUS payload: counted half-steps, not encoder feedback. */
} LinkMessage;
uint16_t LinkProtocol_Crc(const uint8_t *data, size_t size);
void LinkProtocol_Encode(uint8_t *out, const LinkMessage *m);
int LinkProtocol_Decode(LinkMessage *m, const uint8_t *data);
int LinkProtocol_IsNewSequence(uint16_t next, uint16_t previous);
#endif
