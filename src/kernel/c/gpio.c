#include "gpio.h"
#include "printf.h"

void gpio_pin_set_func(uint8_t pinNumber, GpioFunc_t func){
    uint8_t bit_start = (pinNumber * 3) % 30;
    uint8_t reg = pinNumber / 10;

    uint32_t selector = REGS_GPIO->func_select[reg];
    selector &= ~(7 << bit_start);
    selector |= (func << bit_start);

    REGS_GPIO->func_select[reg] = selector;
}

void gpio_set_pin_high(uint8_t pinNumber){
    uint32_t idx = pinNumber / 32;
    uint32_t offset = pinNumber % 32;
    REGS_GPIO->output_set.data[idx] = (1 << offset);
}

void gpio_set_pin_low(uint8_t pinNumber){
    uint32_t idx = pinNumber / 32;
    uint32_t offset = pinNumber % 32;
    REGS_GPIO->output_clear.data[idx] = (1 << offset);
}

void gpio_pin_enable(uint8_t pinNumber){
    REGS_GPIO->pupd_enable = 0;
    delay(150);
    REGS_GPIO->pupd_clocks_enable[pinNumber / 32] = 1 << (pinNumber % 32);
    delay(150);
    REGS_GPIO->pupd_enable = 0;
    REGS_GPIO->pupd_clocks_enable[pinNumber / 32] = 0;
}

void pulse(uint32_t pin, bool on){
    if(on){
        PDEBUG("Turning %d off...\n", pin);
        gpio_set_pin_low(pin);
    }else{
        PDEBUG("Turning %d on...\n", pin);
        gpio_set_pin_high(pin);
    }
}