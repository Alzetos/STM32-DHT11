#ifndef DHT11_H
#define DHT11_H
#include <stdint.h>
typedef enum { DHT_OK, DHT_TIMEOUT, DHT_CHECKSUM, DHT_RANGE } DhtStatus;
typedef struct { uint16_t humidity10; int16_t temperature10; } DhtReading;
DhtStatus DHT11_Read(DhtReading *reading);
void DelayUs(uint16_t us);
#endif
