#ifndef __GPIO_H__
#define __GPIO_H__

#include "peripherals/gpio.h"
#include "utils/utils.h"
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

void gpio_pin_set_func(u8 pinNumber, GpioFunc_t func);
void gpio_pin_enable(u8 pinNumber);
void gpio_set_pin_high(u8 pinNumber);
void gpio_set_pin_low(u8 pinNumber);
void pulse(u32 pin, bool on);

#endif