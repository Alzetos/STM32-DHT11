#ifndef UART_LINK_H
#define UART_LINK_H
#include "link_protocol.h"
typedef struct {
  uint32_t tx_frames, rx_frames, rx_bytes, errors;
  uint16_t last_size, last_uart_flags;
  uint8_t last_rx[LINK_PACKET_SIZE];
} UartLinkDiagnostics;
void UartLink_Init(void);
void UartLink_IRQHandler(void);
int UartLink_Send(const uint8_t *packet);
int UartLink_Receive(uint8_t *packet);
int UartLink_Exchange(const uint8_t *tx, uint8_t *rx);
uint32_t UartLink_ErrorCount(void);
void UartLink_GetDiagnostics(UartLinkDiagnostics *out);
#endif
