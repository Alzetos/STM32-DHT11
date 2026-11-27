#ifndef UART_MOCK_MAIN_H
#define UART_MOCK_MAIN_H
#include <stdint.h>
typedef struct { volatile uint32_t SR,DR,BRR,CR1,CR2,CR3; } USART_TypeDef;
typedef struct { volatile uint32_t MAPR; } AFIO_TypeDef;
typedef struct { uint32_t odr; } GPIO_TypeDef;
typedef struct { uint32_t Pin,Mode,Pull,Speed; } GPIO_InitTypeDef;
extern USART_TypeDef test_uart;
extern AFIO_TypeDef test_afio;
extern GPIO_TypeDef test_a;
extern uint32_t test_primask, test_clocks, test_mode[16], test_pull[16], test_nvic;
#define USART2 (&test_uart)
#define AFIO (&test_afio)
#define GPIOA (&test_a)
#define AFIO_MAPR_SWJ_CFG (7U<<24)
#define AFIO_MAPR_SWJ_CFG_JTAGDISABLE (2U<<24)
#define AFIO_MAPR_USART2_REMAP (1U<<3)
#define MODIFY_REG(reg,mask,value) ((reg)=((reg)&~(mask))|(value))
#define USART_SR_PE 1U
#define USART_SR_FE 2U
#define USART_SR_NE 4U
#define USART_SR_ORE 8U
#define USART_SR_RXNE (1U<<5)
#define USART_SR_TC (1U<<6)
#define USART_SR_TXE (1U<<7)
#define USART_CR1_RE (1U<<2)
#define USART_CR1_TE (1U<<3)
#define USART_CR1_RXNEIE (1U<<5)
#define USART_CR1_UE (1U<<13)
#define GPIO_PIN_2 (1U<<2)
#define GPIO_PIN_3 (1U<<3)
#define GPIO_MODE_INPUT 1U
#define GPIO_MODE_AF_PP 2U
#define GPIO_PULLUP 1U
#define GPIO_SPEED_FREQ_HIGH 3U
#define GPIO_PIN_SET 1U
#define USART2_IRQn 38U
#define __HAL_RCC_GPIOA_CLK_ENABLE() (test_clocks|=1)
#define __HAL_RCC_AFIO_CLK_ENABLE() (test_clocks|=2)
#define __HAL_RCC_USART2_CLK_ENABLE() (test_clocks|=4)
#define __HAL_RCC_USART2_FORCE_RESET() TestResetUart()
#define __HAL_RCC_USART2_RELEASE_RESET() ((void)0)
#define __get_PRIMASK() test_primask
#define __disable_irq() (test_primask=1)
#define __set_PRIMASK(value) (test_primask=(value))
#define __DMB() ((void)0)
void TestResetUart(void);
void HAL_GPIO_Init(GPIO_TypeDef *,GPIO_InitTypeDef *);
void HAL_GPIO_WritePin(GPIO_TypeDef *,uint32_t,uint32_t);
void HAL_NVIC_SetPriority(uint32_t,uint32_t,uint32_t);
void HAL_NVIC_EnableIRQ(uint32_t);
uint32_t HAL_RCC_GetPCLK1Freq(void);
uint32_t HAL_GetTick(void);
#endif
