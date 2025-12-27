#ifndef __EMMC_CLOCK_H__
#define __EMMC_CLOCK_H__

#include "mailbox/mailbox.h"
#include "macros.h"
#include "utils/timer.h"

bool wait_reg_mask(reg32_t* reg, uint32_t mask, bool set, uint32_t timeout);
uint32_t get_clock_divider(uint32_t base_clock, uint32_t target_rate);
bool switch_clock_rate(uint32_t base_clock, uint32_t target_rate);
bool emmc_setup_clock();

#endif