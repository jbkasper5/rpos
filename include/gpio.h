#ifndef __GPIO_H__
#define __GPIO_H__

#include "peripherals.h"

void uart_init();
void putc(char c);
void puts(char* s);

void printf(const char* format_str, ...);

#endif