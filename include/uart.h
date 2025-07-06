#ifndef __UART_H__
#define __UART_H__

void uart_init();
void uart_putc(char c);
char uart_getc();
void uart_puts(const char* str);

#endif