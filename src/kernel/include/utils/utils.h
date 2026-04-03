#ifndef __UTILS_H__
#define __UTILS_H__

#include "macros.h"
#include "system/process.h"

// assembly functions
void delay(uint64_t ticks);

void put32(uint64_t address, uint32_t value);
uint32_t get32(uint64_t address);

void put64(uint64_t address, uint64_t value);
uint64_t get64(uint64_t address);

uint32_t get_el();
uint8_t mmu_enabled();

int log2_pow2(uint64_t num);
int strcmp(const char* str1, const char* str2);
int strlen(const char* str);

pcb_t* get_active_pcb();
void set_active_pcb(pcb_t* pcb);

#endif