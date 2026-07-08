#ifndef __TIMER_TYPES_H__
#define __TIMER_TYPES_H__

#include "macros.h"

struct timer_regs{
    reg32_t control_status;
    reg32_t counter_lo;
    reg32_t counter_hi;
    reg32_t compare[4];
};

#endif
