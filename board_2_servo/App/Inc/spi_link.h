#ifndef SPI_LINK_H
#define SPI_LINK_H
#include "spi_protocol.h"
void SpiLink_Init(const uint8_t *initial_status);
int SpiLink_MasterExchange(const uint8_t *tx, uint8_t *rx);
int SpiLink_SlaveReceive(uint8_t *packet);
void SpiLink_SlavePublish(const uint8_t *status);
uint32_t SpiLink_ErrorCount(void);
void SpiLink_NssRisingIRQ(void);
#endif
