#ifndef __PRINT_H__
#define __PRINT_H__

#include <stdarg.h>
#include "mini_uart.h"
#include "macros.h"

char nibble_to_char(char nibble);
void printf(char* format_str, ...);
char* _int_to_str(int num, char is_long);
char* _to_hex_str(unsigned long num);
uint64_t get_sp();

#define INT_SIZE    (sizeof(int) * 8)
#define FLOAT_SIZE  (sizeof(float) * 8)
#define LONG_SIZE   (sizeof(long) * 8)

#endif