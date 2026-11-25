#ifndef SPI_PROTOCOL_H
#define SPI_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>
#define SPI_PACKET_SIZE 14U
#define SPI_TYPE_MEASUREMENT 0x10U
#define SPI_TYPE_POLL 0x20U
#define SPI_TYPE_HELLO 0x30U
#define SPI_TYPE_STATUS 0x90U
#define SPI_DATA_VALID 0x01U
enum {
  SPI_STATUS_HAVE_MEASUREMENT = 1U, SPI_STATUS_DATA_VALID = 2U,
  SPI_STATUS_STALE = 4U, SPI_STATUS_SENSOR_ERROR = 8U,
  SPI_STATUS_RX_ERROR = 16U, SPI_STATUS_OPEN = 32U,
  SPI_STATUS_SESSION_READY = 64U
};
typedef struct {
  uint8_t type, flags, detail;
  uint16_t sequence;
  int16_t temperature10;
  uint16_t humidity10;
  uint16_t pulse_us, age_seconds; /* STATUS payload replaces temperature/humidity. */
} SpiMessage;
uint16_t SpiProtocol_Crc(const uint8_t *data, size_t size);
void SpiProtocol_Encode(uint8_t *out, const SpiMessage *m);
int SpiProtocol_Decode(SpiMessage *m, const uint8_t *data);
int SpiProtocol_IsNewSequence(uint16_t next, uint16_t previous);
#endif
