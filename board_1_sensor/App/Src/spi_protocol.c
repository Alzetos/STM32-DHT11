#include "spi_protocol.h"
#include <string.h>
static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static uint16_t get16(const uint8_t *p) { return (uint16_t)((uint16_t)p[0] << 8 | p[1]); }
uint16_t SpiProtocol_Crc(const uint8_t *p, size_t size) {
  uint16_t crc = 0xFFFFU;
  while (size--) {
    crc ^= (uint16_t)*p++ << 8;
    for (unsigned i = 0; i < 8; ++i)
      crc = (uint16_t)((crc & 0x8000U) ? ((uint32_t)crc << 1) ^ 0x1021U : (uint32_t)crc << 1);
  }
  return crc;
}
void SpiProtocol_Encode(uint8_t *out, const SpiMessage *m) {
  memset(out, 0, SPI_PACKET_SIZE);
  out[0] = 0xA5; out[1] = 0x5A; out[2] = 1; out[3] = m->type;
  put16(out + 4, m->sequence);
  if (m->type == SPI_TYPE_MEASUREMENT) {
    put16(out + 6, (uint16_t)m->temperature10); put16(out + 8, m->humidity10);
  } else if (m->type == SPI_TYPE_STATUS) {
    put16(out + 6, m->pulse_us); put16(out + 8, m->age_seconds);
  }
  out[10] = m->flags; out[11] = m->detail;
  put16(out + 12, SpiProtocol_Crc(out, 12));
}
int SpiProtocol_Decode(SpiMessage *m, const uint8_t *p) {
  if (p[0] != 0xA5 || p[1] != 0x5A || p[2] != 1 ||
      get16(p + 12) != SpiProtocol_Crc(p, 12)) return 0;
  memset(m, 0, sizeof *m);
  m->type = p[3]; m->sequence = get16(p + 4); m->flags = p[10]; m->detail = p[11];
  if (m->type == SPI_TYPE_MEASUREMENT) {
    uint16_t raw = get16(p + 6);
    m->temperature10 = (int16_t)((raw & 0x8000U) ? (int32_t)raw - 65536 : raw);
    m->humidity10 = get16(p + 8);
    if ((m->flags & ~SPI_DATA_VALID) || m->detail) return 0;
    if ((m->flags & SPI_DATA_VALID) &&
        (m->temperature10 < -400 || m->temperature10 > 800 || m->humidity10 > 1000)) return 0;
  } else if (m->type == SPI_TYPE_POLL || m->type == SPI_TYPE_HELLO) {
    if ((m->type == SPI_TYPE_POLL && m->sequence) ||
        get16(p + 6) || get16(p + 8) || m->flags || m->detail) return 0;
  } else if (m->type == SPI_TYPE_STATUS) {
    m->pulse_us = get16(p + 6); m->age_seconds = get16(p + 8);
    if ((m->flags & ~127U) || (m->detail & ~3U) || m->pulse_us < 1000 || m->pulse_us > 2000) return 0;
  } else return 0;
  return 1;
}
int SpiProtocol_IsNewSequence(uint16_t next, uint16_t previous) {
  uint16_t delta = (uint16_t)(next - previous);
  return delta != 0U && delta < 0x8000U;
}
