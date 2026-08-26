#ifndef __EMMC_CLOCK_H__
#define __EMMC_CLOCK_H__

#include "mailbox/mailbox.h"
#include "macros.h"
#include "utils/timer.h"

bool wait_reg_mask(reg32_t* reg, u32 mask, bool set, u32 timeout);
u32 get_clock_divider(u32 base_clock, u32 target_rate);
bool switch_clock_rate(u32 base_clock, u32 target_rate);
bool emmc_setup_clock();

#endif