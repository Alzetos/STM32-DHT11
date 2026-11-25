#ifndef DHT11_H
#define DHT11_H
#include <stdint.h>
typedef enum { DHT_OK, DHT_TIMEOUT, DHT_CHECKSUM, DHT_RANGE } DhtStatus;
typedef struct { uint16_t humidity10; int16_t temperature10; } DhtReading;
typedef enum {
  DHT_DIAG_OK, DHT_DIAG_IDLE_LOW, DHT_DIAG_RELEASE_LOW,
  DHT_DIAG_NO_REPLY, DHT_DIAG_RESPONSE_LOW, DHT_DIAG_RESPONSE_HIGH,
  DHT_DIAG_BIT_LOW, DHT_DIAG_BIT_HIGH, DHT_DIAG_CHECKSUM, DHT_DIAG_RANGE
} DhtDiagnosticStage;
typedef struct {
  DhtDiagnosticStage stage;
  uint8_t idle_high;
  uint8_t bits_received;
  uint16_t response_low_us, response_high_us;
  uint8_t bytes[5];
} DhtDiagnostics;
const DhtDiagnostics *DHT11_GetDiagnostics(void);
const char *DHT11_DiagnosticText(void);
DhtStatus DHT11_Read(DhtReading *reading);
void DelayUs(uint16_t us);
#endif
