#ifndef DISPLAY_H
#define DISPLAY_H
#include <stdint.h>
void Display_Init(void);
void Display_Line(uint8_t row, const char *text);
#endif
