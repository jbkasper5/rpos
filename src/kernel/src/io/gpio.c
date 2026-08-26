#include "io/gpio.h"
#include "io/kprintf.h"

void gpio_pin_set_func(u8 pinNumber, GpioFunc_t func){
    u8 bit_start = (pinNumber * 3) % 30;
    u8 reg = pinNumber / 10;

    u32 selector = REGS_GPIO->func_select[reg];
    selector &= ~(7 << bit_start);
    selector |= (func << bit_start);

    REGS_GPIO->func_select[reg] = selector;
}

void gpio_set_pin_high(u8 pinNumber){
    u32 idx = pinNumber / 32;
    u32 offset = pinNumber % 32;
    REGS_GPIO->output_set.data[idx] = (1 << offset);
}

void gpio_set_pin_low(u8 pinNumber){
    u32 idx = pinNumber / 32;
    u32 offset = pinNumber % 32;
    REGS_GPIO->output_clear.data[idx] = (1 << offset);
}

void gpio_pin_enable(u8 pinNumber){
    REGS_GPIO->pupd_enable = 0;
    delay(150);
    REGS_GPIO->pupd_clocks_enable[pinNumber / 32] = 1 << (pinNumber % 32);
    delay(150);
    REGS_GPIO->pupd_enable = 0;
    REGS_GPIO->pupd_clocks_enable[pinNumber / 32] = 0;
}

void pulse(u32 pin, bool on){
    if(on){
        DEBUG("Turning %d off...\n", pin);
        gpio_set_pin_low(pin);
    }else{
        DEBUG("Turning %d on...\n", pin);
        gpio_set_pin_high(pin);
    }
}