#ifndef __PRINT_H__
#define __PRINT_H__

#include <stdarg.h>
#include "uart.h"
#include "macros.h"

char nibble_to_char(char nibble);
void my_putchar(char msg);

void printl(long msg);
void printaddr(long msg);
void printf(char* format_str, ...);

#define TEMP_BUFSIZE    200
#define INT_SIZE    (sizeof(int) * 8)
#define FLOAT_SIZE  (sizeof(float) * 8)
#define LONG_SIZE   (sizeof(long) * 8)

#endif