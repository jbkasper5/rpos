#include "io/mini_uart.h"
#include "io/gpio.h"
#include "io/kprintf.h"
#include "io/cli.h"
#include "io/lcd.h"

#include "utils/utils.h"

#define TXD 14
#define RXD 15

uint8_t USE_PANEL = TRUE;

void disable_panel(){
    USE_PANEL = FALSE;
}

void uart_putc(char c){
    if (c == '\n'){
        while(!(REGS_AUX->mu_lsr & 0x20));
        REGS_AUX->mu_io = '\r';
    }
    while(!(REGS_AUX->mu_lsr & 0x20));
    REGS_AUX->mu_io = c;

    if(panel_ready() && USE_PANEL) print_glyph(c);
}

char uart_getc(){
    while(!(REGS_AUX->mu_lsr & 1));
    return REGS_AUX->mu_io & 0xFF;
}

void uart_puts(const char* str){
    while(*str){
        uart_putc(*str++);
    }
}