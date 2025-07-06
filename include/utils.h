#ifndef __UTILS_H__
#define __UTILS_H__

#include "macros.h"

extern void delay(uint64_t ticks);
extern void put32(uint64_t address, uint32_t value);
extern uint32_t get32(uint64_t address);
extern uint32_t get_el();

#endif