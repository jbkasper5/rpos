#ifndef __UTILS_H__
#define __UTILS_H__

#include "macros.h"
#include "system/process.h"

// assembly functions
void delay(u64 ticks);

void put32(u64 address, u32 value);
u32 get32(u64 address);

void put64(u64 address, u64 value);
u64 get64(u64 address);

u32 get_el();
u8 mmu_enabled();

int log2_pow2(u64 num);
int strcmp(const char* str1, const char* str2);
int strlen(const char* str);

pcb_t* get_active_pcb();
void set_active_pcb(pcb_t* pcb);

#endif