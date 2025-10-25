#ifndef __TIMER_H__
#define __TIMER_H__

#include "macros.h"

void prime_physical_timer();
void physical_timer_enable();
uint64_t read_timer();

void timer_init();
void handle_timer_1();
void handle_timer_3();
void handle_physical_timer();
uint64_t timer_get_ticks();
void timer_sleep(uint32_t milliseconds);

#endif