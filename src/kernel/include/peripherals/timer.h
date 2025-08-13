#ifndef __PERIPHERALS_TIMER_H__
#define __PERIPHERALS_TIMER_H__

#include "peripherals/irq.h"
#include "macros.h"

#define CLOCKHZ         1000000

struct timer_regs{
    reg32_t control_status;
    reg32_t counter_lo;
    reg32_t counter_hi;
    reg32_t compare[4];
};

#define REGS_TIMER ((struct timer_regs*) (PBASE + 0x00003000))

#endif