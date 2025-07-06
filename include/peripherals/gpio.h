#ifndef __PERIPHERAL_GPIO_H__
#define __PERIPHERAL_GPIO_H__

#include "macros.h"
#include "peripherals/base.h"
#include "peripherals/uart.h"

struct GpioPinData{
    reg32_t reserved;
    reg32_t data[2];
};

struct GpioRegs{
    reg32_t func_select[6];
    struct GpioPinData output_set;
    struct GpioPinData output_clear;
    struct GpioPinData level;
    struct GpioPinData ev_detect_status;
    struct GpioPinData re_detect_enable;
    struct GpioPinData fe_detect_enable;
    struct GpioPinData hi_detect_enable;
    struct GpioPinData lo_detect_enable;
    struct GpioPinData async_re_detect;
    struct GpioPinData async_fe_detect;
    reg32_t reserved;
    reg32_t pupd_enable;
    reg32_t pupd_enable_clocks[2];
};

#define REGS_GPIO ((struct GpioRegs*)(PERIPHERAL_BASE + 0x00200000))

#endif