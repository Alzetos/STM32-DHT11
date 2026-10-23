#include "main.h"
#include "dht11.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

TIM_HandleTypeDef htim2;
enum { NORMAL, IDLE_LOW, RELEASE_LOW, NO_REPLY, SHORT_RESPONSE_LOW,
       SHORT_RESPONSE_HIGH, STUCK_BIT_LOW, STUCK_BIT_HIGH };
static uint32_t us, release_at, low_start;
static int mode, released, active;
static unsigned rise_delay;
static uint8_t frame[5];

uint16_t fake_counter(void) { return (uint16_t)us++; }
void HAL_Delay(uint32_t ms) { us += ms * 1000U; }
void HAL_GPIO_WritePin(int port, int pin, GPIO_PinState state)
{
  assert(port == DHT_DATA_GPIO_Port && pin == DHT_DATA_Pin);
  if (state == GPIO_PIN_RESET) { released = 0; low_start = us; }
  else {
    assert(!released && us - low_start >= 18000U);
    released = 1; active = 1; release_at = us;
  }
}
GPIO_PinState HAL_GPIO_ReadPin(int port, int pin)
{
  assert(port == DHT_DATA_GPIO_Port && pin == DHT_DATA_Pin);
  if (mode == IDLE_LOW || !released) return GPIO_PIN_RESET;
  if (!active) return GPIO_PIN_SET;
  uint32_t t = us - release_at;
  if (mode == RELEASE_LOW) return GPIO_PIN_RESET;
  if (t < rise_delay) return GPIO_PIN_RESET;
  if (mode == NO_REPLY || t < 30U) return GPIO_PIN_SET;
  t -= 30;
  unsigned low = mode == SHORT_RESPONSE_LOW ? 30U : 80U;
  if (t < low) return GPIO_PIN_RESET;
  t -= low;
  unsigned high = mode == SHORT_RESPONSE_HIGH ? 30U : 80U;
  if (t < high) return GPIO_PIN_SET;
  t -= high;
  for (unsigned bit = 0; bit < 40; bit++) {
    if (mode == STUCK_BIT_LOW && bit == 7) return GPIO_PIN_RESET;
    if (t < 50) return GPIO_PIN_RESET;
    t -= 50;
    if (mode == STUCK_BIT_HIGH && bit == 10) return GPIO_PIN_SET;
    high = frame[bit / 8] & (128U >> (bit % 8)) ? 70U : 28U;
    if (t < high) return GPIO_PIN_SET;
    t -= high;
  }
  return t < 50 ? GPIO_PIN_RESET : GPIO_PIN_SET;
}
static void reset_bus(int next_mode)
{
  mode = next_mode; us = 45000; released = 1; active = 0; rise_delay = 0;
  frame[0] = 60; frame[1] = 0; frame[2] = 25; frame[3] = 0; frame[4] = 85;
}
static void timeout_case(int next_mode, DhtDiagnosticStage stage)
{
  reset_bus(next_mode);
  DhtReading reading = {123, 456};
  uint32_t start = us;
  assert(DHT11_Read(&reading) == DHT_TIMEOUT);
  assert(DHT11_GetDiagnostics()->stage == stage);
  assert(us - start < 27000); /* bounded waits, including the 20 ms request */
  assert(reading.humidity10 == 123 && reading.temperature10 == 456);
  assert(strlen(DHT11_DiagnosticText()) <= 26);
}
int main(void)
{
  DhtReading reading = {0};
  reset_bus(NORMAL);
  assert(DHT11_Read(&reading) == DHT_OK);
  assert(reading.humidity10 == 600 && reading.temperature10 == 250);
  assert(DHT11_GetDiagnostics()->bits_received == 40);
  assert(DHT11_GetDiagnostics()->stage == DHT_DIAG_OK);
  assert(!memcmp(frame, DHT11_GetDiagnostics()->bytes, 5));
  /* Counter wraps during this frame; a slow pull-up must not look like a reply. */
  reset_bus(NORMAL); rise_delay = 8;
  assert(DHT11_Read(&reading) == DHT_OK);
  timeout_case(IDLE_LOW, DHT_DIAG_IDLE_LOW);
  timeout_case(RELEASE_LOW, DHT_DIAG_RELEASE_LOW);
  timeout_case(NO_REPLY, DHT_DIAG_NO_REPLY);
  timeout_case(SHORT_RESPONSE_LOW, DHT_DIAG_RESPONSE_LOW);
  timeout_case(SHORT_RESPONSE_HIGH, DHT_DIAG_RESPONSE_HIGH);
  timeout_case(STUCK_BIT_LOW, DHT_DIAG_BIT_LOW);
  assert(DHT11_GetDiagnostics()->bits_received == 7);
  timeout_case(STUCK_BIT_HIGH, DHT_DIAG_BIT_HIGH);
  assert(DHT11_GetDiagnostics()->bits_received == 10);
  reset_bus(NORMAL); frame[4]--;
  assert(DHT11_Read(&reading) == DHT_CHECKSUM);
  assert(DHT11_GetDiagnostics()->stage == DHT_DIAG_CHECKSUM);
  reset_bus(NORMAL); frame[0] = 101; frame[4] = 126;
  assert(DHT11_Read(&reading) == DHT_RANGE);
  assert(DHT11_GetDiagnostics()->stage == DHT_DIAG_RANGE);
  puts("PASS DHT11: valid packet, counter wrap, slow pull-up, idle/release errors,");
  puts("            reply/bit timeouts, checksum/range errors, diagnostic stages.");
}
