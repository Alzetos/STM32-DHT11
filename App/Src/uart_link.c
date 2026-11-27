#include "board_role.h"
#if BOARD_ROLE != BOARD_STANDALONE
#include "main.h"
#include "uart_link.h"
#include "uart_framer.h"
#include <string.h>
#define RING_SIZE 256U
#define UART_ERRORS (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)
static uint8_t ring[RING_SIZE];
static uint32_t arrived[RING_SIZE];
static volatile uint16_t head, tail;
static volatile uint32_t hardware_errors, fault_epoch, received_bytes;
static uint32_t seen_epoch;
static UartFramer framer;
static UartLinkDiagnostics diagnostics;

void UartLink_Init(void) {
  __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_USART2_CLK_ENABLE();
  /* USART2 default pins: TX PA2, RX PA3; preserve SWD explicitly. */
  MODIFY_REG(AFIO->MAPR, AFIO_MAPR_SWJ_CFG | AFIO_MAPR_USART2_REMAP,
             AFIO_MAPR_SWJ_CFG_JTAGDISABLE);
  __HAL_RCC_USART2_FORCE_RESET(); __HAL_RCC_USART2_RELEASE_RESET();
  GPIO_InitTypeDef pin = {0};
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
  pin.Pin = GPIO_PIN_2; pin.Mode = GPIO_MODE_AF_PP;
  pin.Speed = GPIO_SPEED_FREQ_HIGH; HAL_GPIO_Init(GPIOA, &pin);
  pin.Pin = GPIO_PIN_3; pin.Mode = GPIO_MODE_INPUT; pin.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &pin);
  head = tail = 0; hardware_errors = fault_epoch = received_bytes = seen_epoch = 0;
  memset(&diagnostics, 0, sizeof diagnostics); UartFramer_Init(&framer);
  USART2->BRR = (HAL_RCC_GetPCLK1Freq() + 4800U) / 9600U;
  USART2->CR2 = 0; USART2->CR3 = 0;
  USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
  HAL_NVIC_SetPriority(USART2_IRQn, 5, 0); HAL_NVIC_EnableIRQ(USART2_IRQn);
}
void UartLink_IRQHandler(void) {
  uint32_t status = USART2->SR;
  if (!(status & (USART_SR_RXNE | UART_ERRORS))) return;
  /* Reading SR followed by DR clears RXNE and PE/FE/NE/ORE on STM32F1. */
  uint8_t byte = (uint8_t)USART2->DR;
  diagnostics.last_uart_flags = (uint16_t)status;
  if (status & UART_ERRORS) { ++hardware_errors; ++fault_epoch; return; }
  ++received_bytes;
  uint16_t next = (uint16_t)((head + 1U) % RING_SIZE);
  if (next == tail) { ++hardware_errors; ++fault_epoch; return; }
  ring[head] = byte; arrived[head] = HAL_GetTick(); __DMB(); head = next;
}
int UartLink_Receive(uint8_t *packet) {
  for (;;) {
    uint32_t mask = __get_PRIMASK(); __disable_irq();
    if (seen_epoch != fault_epoch) {
      /* A lost byte invalidates queued fragments; wait for the next request. */
      seen_epoch = fault_epoch; tail = head; framer.used = 0;
    }
    if (head == tail) { __set_PRIMASK(mask); return 0; }
    uint8_t byte = ring[tail]; uint32_t when = arrived[tail]; tail = (uint16_t)((tail + 1U) % RING_SIZE);
    __set_PRIMASK(mask);
    if (UartFramer_Feed(&framer, byte, when, packet)) {
      ++diagnostics.rx_frames; diagnostics.last_size = LINK_PACKET_SIZE;
      memcpy(diagnostics.last_rx, packet, LINK_PACKET_SIZE); return 1;
    }
  }
}
static int wait_tx(uint32_t flag, uint32_t start) {
  while (!(USART2->SR & flag)) {
    if ((uint32_t)(HAL_GetTick() - start) >= 50U) {
      ++hardware_errors; return 0;
    }
  }
  return 1;
}
int UartLink_Send(const uint8_t *packet) {
  uint32_t start = HAL_GetTick();
  for (unsigned i = 0; i < LINK_PACKET_SIZE; ++i) {
    if (!wait_tx(USART_SR_TXE, start)) return 0;
    USART2->DR = packet[i];
  }
  if (!wait_tx(USART_SR_TC, start)) return 0;
  ++diagnostics.tx_frames; return 1;
}
int UartLink_Exchange(const uint8_t *tx, uint8_t *rx) {
  /* Request/reply only: the receiver never sends unsolicited status. Flush a
   * late previous reply before a new request; sequence checks still apply. */
  uint32_t mask = __get_PRIMASK(); __disable_irq();
  tail = head; seen_epoch = fault_epoch; framer.used = 0; __set_PRIMASK(mask);
  if (!UartLink_Send(tx)) return 0;
  uint32_t start = HAL_GetTick();
  do {
    if (UartLink_Receive(rx)) return 1;
  } while ((uint32_t)(HAL_GetTick() - start) < 150U);
  return 0;
}
uint32_t UartLink_ErrorCount(void) { return hardware_errors + framer.errors; }
void UartLink_GetDiagnostics(UartLinkDiagnostics *out) {
  uint32_t mask = __get_PRIMASK(); __disable_irq();
  *out = diagnostics; out->rx_bytes = received_bytes;
  out->errors = hardware_errors + framer.errors; __set_PRIMASK(mask);
}
#endif
