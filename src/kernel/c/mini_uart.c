#include "mini_uart.h"
#include "gpio.h"
#include "utils.h"

#define TXD 14
#define RXD 15

void uart_init(){
    gpio_pin_set_func(TXD, GFAlt5);
    gpio_pin_set_func(RXD, GFAlt5);

    gpio_pin_enable(TXD);
    gpio_pin_enable(RXD);

    REGS_AUX->enables = 1;
    REGS_AUX->mu_control = 0;

    REGS_AUX->mu_ier = 0xD;
    REGS_AUX->mu_lcr = 3;
    REGS_AUX->mu_mcr = 0;
    REGS_AUX->mu_baud_rate = 541;
    REGS_AUX->mu_control = 3;

    uart_putc('\n');
    uart_putc('\n');
}

void uart_putc(char c){
    if (c == '\n'){
        while(!(REGS_AUX->mu_lsr & 0x20));
        REGS_AUX->mu_io = '\r';
    }
    while(!(REGS_AUX->mu_lsr & 0x20));
    REGS_AUX->mu_io = c;
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