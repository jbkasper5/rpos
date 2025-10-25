#ifndef __GPIO_H__
#define __GPIO_H__

#include "peripherals/gpio.h"
#include "utils.h"
#include "peripherals/aux.h"

typedef enum _GpioFunc {
    GFInput = 0,
    GFOutput = 1,
    GFAlt0 = 4,
    GFAlt1 = 5,
    GFAlt2 = 6,
    GFAlt3 = 7,
    GFAlt4 = 3,
    GFAlt5 = 2
} GpioFunc_t;

void gpio_pin_set_func(uint8_t pinNumber, GpioFunc_t func);
void gpio_pin_enable(uint8_t pinNumber);
void gpio_set_pin_high(uint8_t pinNumber);
void gpio_set_pin_low(uint8_t pinNumber);
void pulse(uint32_t pin, bool on);

#endif