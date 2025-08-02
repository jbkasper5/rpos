#ifndef __UTILS_H__
#define __UTILS_H__

// assembly functions
void delay(uint64_t ticks);
void put32(uint64_t address, uint32_t value);
uint32_t get32(uint64_t address);
uint32_t get_el();

#endif