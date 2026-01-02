#ifndef __UTILS_H__
#define __UTILS_H__

#include "macros.h"

// assembly functions
void delay(uint64_t ticks);

void put32(uint64_t address, uint32_t value);
uint32_t get32(uint64_t address);

void put64(uint64_t address, uint64_t value);
uint64_t get64(uint64_t address);

uint32_t get_el();
uint8_t mmu_enabled();

int log2_pow2(uint64_t num);

#endif