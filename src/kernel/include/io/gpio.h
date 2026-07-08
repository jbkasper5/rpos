#ifndef __GPIO_H__
#define __GPIO_H__

#include "peripherals/gpio.h"
#include "utils/utils.h"
#include "peripherals/aux.h"
#include "types/gpio_types.h"

void gpio_pin_set_func(u8 pinNumber, GpioFunc_t func);
void gpio_pin_enable(u8 pinNumber);
void gpio_set_pin_high(u8 pinNumber);
void gpio_set_pin_low(u8 pinNumber);
void pulse(u32 pin, bool on);

#endif