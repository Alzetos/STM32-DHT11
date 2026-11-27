#include "main.h"
#include "uart_link.h"
#include "uart_framer.h"
#include "greenhouse_client.h"
#include "greenhouse_server.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
USART_TypeDef test_uart;
AFIO_TypeDef test_afio;
GPIO_TypeDef test_a;
uint32_t test_primask,test_clocks,test_mode[16],test_pull[16],test_nvic;
static uint32_t tick;
static const uint8_t *scheduled;
static int injecting;
static void inject(const uint8_t *p, unsigned n);
void TestResetUart(void) {
  memset(&test_uart,0,sizeof test_uart); test_uart.SR=USART_SR_TXE|USART_SR_TC;
}
void HAL_GPIO_Init(GPIO_TypeDef *p,GPIO_InitTypeDef *pin) {
  assert(p==GPIOA);
  for (unsigned i=0;i<16;++i) if (pin->Pin&(1U<<i)) {
    test_mode[i]=pin->Mode; test_pull[i]=pin->Pull;
  }
}
void HAL_GPIO_WritePin(GPIO_TypeDef *p,uint32_t pin,uint32_t state) {
  assert(p==GPIOA && state==GPIO_PIN_SET);p->odr|=pin;
}
void HAL_NVIC_SetPriority(uint32_t irq,uint32_t priority,uint32_t sub) {
  assert(irq==USART2_IRQn && priority==5 && sub==0);
}
void HAL_NVIC_EnableIRQ(uint32_t irq) { test_nvic=irq; }
uint32_t HAL_RCC_GetPCLK1Freq(void) { return 36000000U; }
uint32_t HAL_GetTick(void) {
  ++tick;
  if (scheduled && !injecting) {
    /* Schedule the reply only after the request's Send() has completed. */
    if (++injecting==1) { const uint8_t *p=scheduled; scheduled=0;inject(p,LINK_PACKET_SIZE); }
    injecting=0;
  }
  return tick;
}
static void inject(const uint8_t *p, unsigned n) {
  for (unsigned i=0;i<n;++i) {
    test_uart.DR=p[i];test_uart.SR=USART_SR_TXE|USART_SR_TC|USART_SR_RXNE;
    UartLink_IRQHandler();test_uart.SR=USART_SR_TXE|USART_SR_TC;
  }
}
static int feed(UartFramer *f,const uint8_t *p,unsigned n,uint32_t *now,uint8_t *out) {
  int found=0;
  for(unsigned i=0;i<n;++i)found+=UartFramer_Feed(f,p[i],(*now)++,out);
  return found;
}
static void framer_tests(const uint8_t *packet) {
  UartFramer f; uint8_t out[LINK_PACKET_SIZE],bad[LINK_PACKET_SIZE];uint32_t now=0;
  UartFramer_Init(&f);
  const uint8_t noise[]={0x00,0xFF,0xA5,0xA5,0x00,0x5A};
  assert(!feed(&f,noise,sizeof noise,&now,out));
  assert(feed(&f,packet,14,&now,out)==1 && !memcmp(out,packet,14));
  memcpy(bad,packet,14);bad[13]^=1;
  assert(!feed(&f,bad,14,&now,out));assert(f.errors);
  assert(feed(&f,packet,14,&now,out)==1);
  for(unsigned cut=1;cut<14;++cut) {
    UartFramer_Init(&f);assert(!feed(&f,packet,cut,&now,out));
    assert(feed(&f,packet,14,&now,out)==1); /* truncated frame then valid frame */
  }
  UartFramer_Init(&f);assert(!feed(&f,packet,6,&now,out));
  now+=51;assert(feed(&f,packet,14,&now,out)==1 && f.errors==1);
  UartFramer_Init(&f);now=UINT32_MAX-5;
  assert(feed(&f,packet,14,&now,out)==1); /* tick wrap */
  assert(feed(&f,packet,14,&now,out)==1); /* back-to-back */
}
static void endpoints(void) {
  GreenhouseClient client;GreenhouseServer server;
  UartFramer to_server,to_client;LinkMessage request,decoded,status;
  uint8_t tx[14],rx[14],wire[14];uint32_t now=0;
  GreenhouseClient_Init(&client,42);GreenhouseServer_Init(&server);
  UartFramer_Init(&to_server);UartFramer_Init(&to_client);
  for(unsigned round=0;round<3;++round) {
    if(round)GreenhouseClient_Measurement(&client,251,550,1);
    GreenhouseClient_Request(&client,&request);LinkProtocol_Encode(tx,&request);
    assert(feed(&to_server,tx,14,&now,wire)==1);
    assert(LinkProtocol_Decode(&decoded,wire));
    GreenhouseServer_Receive(&server,&decoded,now);
    GreenhouseServer_Status(&server,now,0,0,&status);LinkProtocol_Encode(tx,&status);
    assert(feed(&to_client,tx,14,&now,rx)==1);
    assert(GreenhouseClient_Reply(&client,rx,now));
    assert(client.ready && !client.pending);
    now+=2200;
  }
  assert(server.control.open && client.remote.sequence==2);
}
int main(void) {
  LinkMessage m={0};uint8_t packet[14],out[14];UartLinkDiagnostics d;
  m.type=LINK_TYPE_HELLO;m.sequence=42;LinkProtocol_Encode(packet,&m);
  framer_tests(packet);endpoints();
  test_afio.MAPR=0x0000123CU;UartLink_Init();
  assert(test_clocks==7 && test_nvic==USART2_IRQn);
  assert(!(test_afio.MAPR&AFIO_MAPR_USART2_REMAP));
  assert((test_afio.MAPR&AFIO_MAPR_SWJ_CFG)==AFIO_MAPR_SWJ_CFG_JTAGDISABLE);
  assert(test_mode[2]==GPIO_MODE_AF_PP && test_mode[3]==GPIO_MODE_INPUT && test_pull[3]==GPIO_PULLUP);
  assert(test_uart.BRR==3750 && test_uart.CR2==0 && test_uart.CR3==0);
  assert(test_uart.CR1==(USART_CR1_UE|USART_CR1_TE|USART_CR1_RE|USART_CR1_RXNEIE));
  inject(packet,14);assert(UartLink_Receive(out) && !memcmp(out,packet,14));
  UartLink_GetDiagnostics(&d);assert(d.rx_frames==1 && d.rx_bytes==14);
  assert(UartLink_Send(packet));assert(test_uart.DR==packet[13]);
  test_uart.SR=0;assert(!UartLink_Send(packet));test_uart.SR=USART_SR_TXE|USART_SR_TC;
  test_uart.SR|=USART_SR_ORE;UartLink_IRQHandler();test_uart.SR=USART_SR_TXE|USART_SR_TC;
  assert(!UartLink_Receive(out));inject(packet,14);assert(UartLink_Receive(out));
  for(unsigned i=0;i<260;++i)inject(packet,1);
  assert(!UartLink_Receive(out));inject(packet,14);assert(UartLink_Receive(out));
  test_primask=1;UartLink_GetDiagnostics(&d);assert(test_primask==1 && d.errors>=3);test_primask=0;
  assert(!UartLink_Exchange(packet,out)); /* no peer: bounded timeout */
  /* An injected complete reply before send completion is still queued and
   * consumed after send; real RX IRQ works throughout the TX wait. */
  scheduled=packet;assert(UartLink_Exchange(packet,out));assert(!memcmp(out,packet,14));
  puts("PASS UART2: PA2/PA3, 9600 8N1, IRQ/ring, errors, timeout, framing/CRC/resync and both endpoints");
}
