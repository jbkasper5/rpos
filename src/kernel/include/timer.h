#ifndef __TIMER_H__
#define __TIMER_H__

#include "macros.h"
#include "datastructures.h"
#include "process.h"

void prime_physical_timer();
void physical_timer_enable();

void prime_virtual_timer(uint64_t cycles);
void virtual_timer_enable();

uint64_t read_physical_timer();
uint64_t read_virtual_timer();

void clear_virtual_timer();

void timer_init();
void handle_timer_1();
void handle_timer_3();
void handle_physical_timer();
void handle_virtual_timer();
uint64_t timer_get_ticks();
void timer_sleep(uint32_t milliseconds);
void timer_nanosleep(uint64_t nanoseconds);

#endif