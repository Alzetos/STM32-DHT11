#include "board_role.h"
#if BOARD_ROLE != BOARD_STANDALONE
#include "main.h"
#include "spi_link.h"
#include <string.h>

static volatile uint32_t errors;
static void reset_spi(void) {
  __HAL_RCC_SPI2_FORCE_RESET();
  __HAL_RCC_SPI2_RELEASE_RESET();
}
#if BOARD_ROLE == BOARD_SENSOR
static void delay_us(uint32_t us) {
  uint32_t start = DWT->CYCCNT, cycles = (SystemCoreClock / 1000000U) * us;
  while ((uint32_t)(DWT->CYCCNT - start) < cycles) __NOP();
}
static void master_start(void) {
  reset_spi();
  /* Mode 0, 8 bits, MSB first; PCLK1/256 = 140625 Hz at 36 MHz. */
  SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR | SPI_CR1_SPE;
}
static int wait_flag(uint32_t flag, int set, uint32_t start) {
  while (((SPI2->SR & flag) != 0U) != set) {
    if ((uint32_t)(HAL_GetTick() - start) >= 20U) return 0;
  }
  return 1;
}
void SpiLink_Init(const uint8_t *initial_status) {
  (void)initial_status;
  __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_SPI2_CLK_ENABLE();
  GPIO_InitTypeDef pin = {0};
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
  pin.Pin = GPIO_PIN_12; pin.Mode = GPIO_MODE_OUTPUT_PP;
  pin.Speed = GPIO_SPEED_FREQ_HIGH; HAL_GPIO_Init(GPIOB, &pin);
  pin.Pin = GPIO_PIN_13 | GPIO_PIN_15; pin.Mode = GPIO_MODE_AF_PP;
  HAL_GPIO_Init(GPIOB, &pin);
  pin.Pin = GPIO_PIN_14; pin.Mode = GPIO_MODE_INPUT; pin.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &pin);
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  master_start();
}
int SpiLink_MasterExchange(const uint8_t *tx, uint8_t *rx) {
  uint32_t start = HAL_GetTick();
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
  delay_us(20);
  for (unsigned i = 0; i < SPI_PACKET_SIZE; ++i) {
    if (!wait_flag(SPI_SR_TXE, 1, start)) goto fail;
    *(__IO uint8_t *)&SPI2->DR = tx[i];
    if (!wait_flag(SPI_SR_RXNE, 1, start)) goto fail;
    rx[i] = *(__IO uint8_t *)&SPI2->DR;
    if (SPI2->SR & (SPI_SR_OVR | SPI_SR_MODF)) goto fail;
  }
  if (!wait_flag(SPI_SR_TXE, 1, start) || !wait_flag(SPI_SR_BSY, 0, start)) goto fail;
  /* Allow the slave RX DMA to finish the last byte before the CS interrupt. */
  delay_us(20);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
  return 1;
fail:
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
  ++errors; master_start();
  return 0;
}
int SpiLink_SlaveReceive(uint8_t *p) { (void)p; return 0; }
void SpiLink_SlavePublish(const uint8_t *p) { (void)p; }
void SpiLink_NssRisingIRQ(void) { }
#else
#define QUEUE_SIZE 4U
static uint8_t rx_dma[SPI_PACKET_SIZE], tx_dma[SPI_PACKET_SIZE];
static uint8_t published[SPI_PACKET_SIZE], queue[QUEUE_SIZE][SPI_PACKET_SIZE];
static volatile uint8_t head, tail;
static void slave_arm(void) {
  /* Called only during initialization or at the physical end of a CS frame.
   * Never change an armed TX buffer from the foreground. */
  reset_spi();
  memcpy(tx_dma, published, SPI_PACKET_SIZE);
  DMA1_Channel4->CCR = 0; DMA1_Channel5->CCR = 0;
  DMA1->IFCR = DMA_IFCR_CGIF4 | DMA_IFCR_CGIF5;
  DMA1_Channel4->CPAR = (uint32_t)&SPI2->DR;
  DMA1_Channel4->CMAR = (uint32_t)rx_dma;
  DMA1_Channel4->CNDTR = SPI_PACKET_SIZE;
  DMA1_Channel5->CPAR = (uint32_t)&SPI2->DR;
  DMA1_Channel5->CMAR = (uint32_t)tx_dma;
  DMA1_Channel5->CNDTR = SPI_PACKET_SIZE;
  /* Normal DMA; PB12 is hardware NSS. Sizes are bytes, with memory increment. */
  DMA1_Channel4->CCR = DMA_CCR_MINC | DMA_CCR_PL | DMA_CCR_EN;
  DMA1_Channel5->CCR = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_PL_1 | DMA_CCR_EN;
  SPI2->CR2 = SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;
  SPI2->CR1 = SPI_CR1_SPE;
}
void SpiLink_Init(const uint8_t *initial_status) {
  __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_SPI2_CLK_ENABLE(); __HAL_RCC_DMA1_CLK_ENABLE();
  GPIO_InitTypeDef pin = {0};
  pin.Pin = GPIO_PIN_13 | GPIO_PIN_15; pin.Mode = GPIO_MODE_INPUT;
  pin.Pull = GPIO_NOPULL; HAL_GPIO_Init(GPIOB, &pin);
  pin.Pin = GPIO_PIN_14; pin.Mode = GPIO_MODE_AF_PP;
  pin.Speed = GPIO_SPEED_FREQ_HIGH; HAL_GPIO_Init(GPIOB, &pin);
  pin.Pin = GPIO_PIN_12; pin.Mode = GPIO_MODE_IT_RISING; pin.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &pin);
  memcpy(published, initial_status, SPI_PACKET_SIZE);
  slave_arm();
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_12);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
void SpiLink_NssRisingIRQ(void) {
  if (!__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_12)) return;
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_12);
  SPI2->CR2 = 0;
  DMA1_Channel4->CCR &= ~DMA_CCR_EN;
  DMA1_Channel5->CCR &= ~DMA_CCR_EN;
  uint32_t remaining = DMA1_Channel4->CNDTR;
  uint32_t fault = (SPI2->SR & (SPI_SR_OVR | SPI_SR_MODF | SPI_SR_RXNE)) |
                   (DMA1->ISR & (DMA_ISR_TEIF4 | DMA_ISR_TEIF5));
  if (!remaining && !fault) {
    uint8_t next = (uint8_t)((head + 1U) % QUEUE_SIZE);
    if (next == tail) ++errors;
    else {
      memcpy(queue[head], rx_dma, SPI_PACKET_SIZE);
      __DMB(); head = next;
    }
  } else ++errors;
  /* Reset also discards a partial frame or prefetched TX bytes. Do not wait
   * for slave BSY: NSS, RX DMA count and error flags delimit this transfer. */
  slave_arm();
}
int SpiLink_SlaveReceive(uint8_t *packet) {
  if (tail == head) return 0;
  memcpy(packet, queue[tail], SPI_PACKET_SIZE);
  __DMB(); tail = (uint8_t)((tail + 1U) % QUEUE_SIZE);
  return 1;
}
void SpiLink_SlavePublish(const uint8_t *status) {
  uint32_t primask = __get_PRIMASK(); __disable_irq();
  memcpy(published, status, SPI_PACKET_SIZE);
  __DMB(); __set_PRIMASK(primask);
}
int SpiLink_MasterExchange(const uint8_t *t, uint8_t *r) { (void)t; (void)r; return 0; }
#endif
uint32_t SpiLink_ErrorCount(void) { return errors; }
#endif
