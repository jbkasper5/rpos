#ifndef __PRINT_H__
#define __PRINT_H__

#include <stdarg.h>
#include "io/mini_uart.h"
#include "macros.h"

void kprintf(char* format_str, ...);
void disable_panel();

#define INT_SIZE    (sizeof(int) * 8)
#define FLOAT_SIZE  (sizeof(float) * 8)
#define LONG_SIZE   (sizeof(long) * 8)

#endif