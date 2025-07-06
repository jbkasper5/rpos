#include "gpio.h"
#include "utils.h"
#include "uart.h"

#define TXD 14
#define RXD 15

void uart_init(){
    REGS_AUX->enables |= 1;
    REGS_AUX->mu_control = 0;
    REGS_AUX->mu_lcr = 3;
    REGS_AUX->mu_mcr = 0;
    REGS_AUX->mu_ier = 0;
    REGS_AUX->mu_iir = 0xc6;

    // for pi4, should be 541 since it's 500 MHz
    REGS_AUX->mu_baud_rate = 270; // 115200 baud rate @ 250 MHz

    gpio_pin_set_func(TXD, GFAlt5);
    gpio_pin_set_func(RXD, GFAlt5);

    unsigned int thing = REGS_GPIO->func_select[1];

    gpio_pin_enable(TXD);
    gpio_pin_enable(RXD);

    REGS_GPIO->pupd_enable = 0;
    REGS_AUX->mu_control = 3;
    
    uart_putc('\r');
    uart_putc('\n');
    uart_putc('\n');
}

void uart_putc(char c){
    while(!(REGS_AUX->mu_lsr & 0x20));
    REGS_AUX->mu_io = c;
}

char uart_getc(){
    while(!(REGS_AUX->mu_lsr & 1));
    return (REGS_AUX->mu_io & 0xFF);
}

void uart_puts(const char* str){
    while(*str){
        if(*str == '\n') uart_putc('\r');
        uart_putc(*str++);
    }
}