#ifndef UART_FRAMER_H
#define UART_FRAMER_H
#include "link_protocol.h"
typedef struct {
  uint8_t buffer[LINK_PACKET_SIZE];
  unsigned used;
  uint32_t last_byte, errors;
} UartFramer;
void UartFramer_Init(UartFramer *f);
int UartFramer_Feed(UartFramer *f, uint8_t byte, uint32_t now, uint8_t *packet);
#endif
