#ifndef __TIMER_H__
#define __TIMER_H__

#include "macros.h"

void timer_init();
void handle_timer_1();
void handle_timer_3();
uint64_t timer_get_ticks();
void timer_sleep(uint32_t milliseconds);

#endif