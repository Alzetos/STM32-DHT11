#include "uart_framer.h"
#include <string.h>
void UartFramer_Init(UartFramer *f) { memset(f, 0, sizeof *f); }
int UartFramer_Feed(UartFramer *f, uint8_t byte, uint32_t now, uint8_t *packet) {
  if (f->used && (uint32_t)(now - f->last_byte) > 50U) {
    f->used = 0; ++f->errors;
  }
  f->last_byte = now;
  f->buffer[f->used++] = byte;
  /* Sliding synchronization also recovers an embedded valid header after a
   * missing byte or a false A5 5A inside noise. Never trust header alone. */
  while (f->used && (f->buffer[0] != 0xA5U ||
         (f->used > 1U && f->buffer[1] != 0x5AU))) {
    --f->used; memmove(f->buffer, f->buffer + 1, f->used);
  }
  if (f->used != LINK_PACKET_SIZE) return 0;
  LinkMessage message;
  if (LinkProtocol_Decode(&message, f->buffer)) {
    memcpy(packet, f->buffer, LINK_PACKET_SIZE); f->used = 0; return 1;
  }
  ++f->errors; --f->used;
  memmove(f->buffer, f->buffer + 1, f->used);
  return 0;
}
