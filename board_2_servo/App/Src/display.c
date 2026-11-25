/* EasyMx PRO v7 CN32: ILI9341, 8080 8-bit, RGB565.
 * PE0..7 data, PE8 reset, PE10 RD, PE11 WR, PE12 RS/DC, PE15 CS.
 * MI0283QT-8 controller reference: manufacturer module specification.
 */
#include "main.h"
#include "display.h"
#include <string.h>
#define RD (1U<<10)
#define WR (1U<<11)
#define DC (1U<<12)
#define CS (1U<<15)
#define RST (1U<<8)
static char cache[10][27];
static void timing(void) { for (volatile unsigned i=0;i<8;i++) __NOP(); }
static void bus(uint8_t b) {
  GPIOE->BSRR = ((uint32_t)(~b & 255U)<<16) | b;
  timing(); GPIOE->BRR = WR; timing(); GPIOE->BSRR = WR; timing();
}
static void cmd(uint8_t c) { GPIOE->BRR=DC; bus(c); GPIOE->BSRR=DC; }
static void data16(uint16_t v) { bus((uint8_t)(v>>8)); bus((uint8_t)v); }
static void reg(uint8_t c,const uint8_t *v,unsigned n) {
  cmd(c); for(unsigned i=0;i<n;i++) bus(v[i]);
}
#define REG(c,...) do { const uint8_t v[]={__VA_ARGS__};reg(c,v,sizeof v); } while(0)
static void window(unsigned x,unsigned y,unsigned w,unsigned h) {
  cmd(0x2a); data16((uint16_t)x);data16((uint16_t)(x+w-1));
  cmd(0x2b); data16((uint16_t)y);data16((uint16_t)(y+h-1));cmd(0x2c);
}
static void fill(unsigned x,unsigned y,unsigned w,unsigned h,uint16_t color) {
  window(x,y,w,h);for(unsigned i=0;i<w*h;i++)data16(color);
}
/* Original compact 5x7 column font for the measurement screen. */
static const struct {char c;uint8_t p[5];} font[]={
 {'0',{0x3e,0x51,0x49,0x45,0x3e}}, {'1',{0,0x42,0x7f,0x40,0}},
 {'2',{0x42,0x61,0x51,0x49,0x46}}, {'3',{0x21,0x41,0x45,0x4b,0x31}},
 {'4',{0x18,0x14,0x12,0x7f,0x10}}, {'5',{0x27,0x45,0x45,0x45,0x39}},
 {'6',{0x3c,0x4a,0x49,0x49,0x30}}, {'7',{1,0x71,9,5,3}},
 {'8',{0x36,0x49,0x49,0x49,0x36}}, {'9',{6,0x49,0x49,0x29,0x1e}},
 {'A',{0x7e,0x11,0x11,0x11,0x7e}}, {'B',{0x7f,0x49,0x49,0x49,0x36}},
 {'C',{0x3e,0x41,0x41,0x41,0x22}}, {'D',{0x7f,0x41,0x41,0x22,0x1c}},
 {'E',{0x7f,0x49,0x49,0x49,0x41}}, {'F',{0x7f,9,9,9,1}},
 {'G',{0x3e,0x41,0x49,0x49,0x7a}}, {'H',{0x7f,8,8,8,0x7f}},
 {'I',{0,0x41,0x7f,0x41,0}}, {'J',{0x20,0x40,0x41,0x3f,1}},
 {'K',{0x7f,8,0x14,0x22,0x41}}, {'L',{0x7f,0x40,0x40,0x40,0x40}},
 {'M',{0x7f,2,0x0c,2,0x7f}}, {'N',{0x7f,4,8,0x10,0x7f}},
 {'O',{0x3e,0x41,0x41,0x41,0x3e}}, {'P',{0x7f,9,9,9,6}},
 {'Q',{0x3e,0x41,0x51,0x21,0x5e}}, {'R',{0x7f,9,0x19,0x29,0x46}},
 {'S',{0x46,0x49,0x49,0x49,0x31}}, {'T',{1,1,0x7f,1,1}},
 {'U',{0x3f,0x40,0x40,0x40,0x3f}}, {'V',{0x1f,0x20,0x40,0x20,0x1f}},
 {'W',{0x7f,0x20,0x18,0x20,0x7f}}, {'X',{0x63,0x14,8,0x14,0x63}},
 {'Y',{7,8,0x70,8,7}}, {'Z',{0x61,0x51,0x49,0x45,0x43}},
 {':',{0,0x36,0x36,0,0}}, {'.',{0,0x60,0x60,0,0}},
 {'%',{0x63,0x13,8,0x64,0x63}}, {'/',{0x20,0x10,8,4,2}},
 {'-',{8,8,8,8,8}}, {'+',{8,8,0x3e,8,8}}, {'!',{0,0,0x5f,0,0}}
};
static void character(unsigned x,unsigned y,char c,uint16_t color) {
  const uint8_t *p=0;if(c>='a'&&c<='z')c-=32;
  for(unsigned i=0;i<sizeof font/sizeof font[0];i++)if(font[i].c==c){p=font[i].p;break;}
  window(x,y,12,16);
  for(unsigned yy=0;yy<16;yy++)for(unsigned xx=0;xx<12;xx++) {
    int lit=p&&xx<10&&yy<14&&(p[xx/2]&(1U<<(yy/2)));
    data16(lit?color:0);
  }
}
void Display_Init(void) {
  /* GPIO configuration is also present in CubeMX IOC. */
  GPIOE->BSRR=CS|RD|WR|DC|RST;
  GPIOE->BRR=RST;HAL_Delay(20);GPIOE->BSRR=RST;HAL_Delay(150);
  GPIOE->BRR=CS;cmd(0x01);HAL_Delay(10);cmd(0x28);
  REG(0xcf,0,0x83,0x30); REG(0xed,0x64,3,0x12,0x81);
  REG(0xe8,0x85,1,0x79); REG(0xcb,0x39,0x2c,0,0x34,2);
  REG(0xf7,0x20);REG(0xea,0,0);REG(0xc0,0x26);REG(0xc1,0x11);
  REG(0xc5,0x35,0x3e);REG(0xc7,0xbe);
  REG(0x36,0x28); /* MV+BGR: 320x240 landscape */
  REG(0x3a,0x55);REG(0xb1,0,0x1b);REG(0xb6,0x0a,0x82,0x27);
  REG(0xf2,0);REG(0x26,1);cmd(0x11);HAL_Delay(150);cmd(0x29);HAL_Delay(20);
  fill(0,0,320,240,0);GPIOE->BSRR=CS;memset(cache,0,sizeof cache);
}
void Display_Line(uint8_t row,const char *text) {
  if(row>=10)return;
  char clipped[27]={0};strncpy(clipped,text,26);
  if(!strcmp(cache[row],clipped))return;
  GPIOE->BRR=CS;
  fill(0,8+row*22,320,20,0);
  for(unsigned i=0;clipped[i];i++)character(4+i*12,8+row*22,clipped[i],row==0?0x07ff:0xffff);
  GPIOE->BSRR=CS;memcpy(cache[row],clipped,sizeof clipped);
}
