#include "io/mini_uart.h"
#include "io/gpio.h"
#include "macros.h"

#define TXD 14
#define RXD 15

static BOOT_FN void gpio_pin_set_func_lo(uint8_t pinNumber, GpioFunc_t func){
    uint8_t bit_start = (pinNumber * 3) % 30;
    uint8_t reg = pinNumber / 10;

    uint32_t selector = REGS_GPIO->func_select[reg];
    selector &= ~(7 << bit_start);
    selector |= (func << bit_start);

    REGS_GPIO->func_select[reg] = selector;
}

static BOOT_FN void gpio_pin_enable_lo(uint8_t pinNumber){
    REGS_GPIO->pupd_enable = 0;
    delay_lo(150);
    REGS_GPIO->pupd_clocks_enable[pinNumber / 32] = 1 << (pinNumber % 32);
    delay_lo(150);
    REGS_GPIO->pupd_enable = 0;
    REGS_GPIO->pupd_clocks_enable[pinNumber / 32] = 0;
}

BOOT_FN void debug_init_lo(){
    // JTAG
    #define TCK 22
    #define TMS 27
    #define TDI 4
    #define TDO 5
    #define TRST 25

    gpio_pin_set_func_lo(TCK, GFAlt4);
    gpio_pin_set_func_lo(TMS, GFAlt4);
    gpio_pin_set_func_lo(TDI, GFAlt4);
    gpio_pin_set_func_lo(TDO, GFAlt4);
    gpio_pin_set_func_lo(TRST, GFAlt4);

    gpio_pin_enable_lo(TCK);
    gpio_pin_enable_lo(TMS);
    gpio_pin_enable_lo(TDI);
    gpio_pin_enable_lo(TDO);
    gpio_pin_enable_lo(TRST);

    int gate = 0;
    while(!gate);
}

BOOT_FN void uart_init_lo(){
    gpio_pin_set_func_lo(TXD, GFAlt5);
    gpio_pin_set_func_lo(RXD, GFAlt5);

    gpio_pin_enable_lo(TXD);
    gpio_pin_enable_lo(RXD);

    REGS_AUX->enables = 1;
    REGS_AUX->mu_control = 0;

    REGS_AUX->mu_ier = 0xD;
    REGS_AUX->mu_lcr = 3;
    REGS_AUX->mu_mcr = 0;
    REGS_AUX->mu_baud_rate = 541;
    REGS_AUX->mu_control = 3;
}