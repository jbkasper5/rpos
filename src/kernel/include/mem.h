#ifndef __MEM_H__
#define __MEM_H__

#include "macros.h"

void* memcpy(void* dest, const void* src, uint32_t bytes);
void* memset(void* ptr, uint64_t value, uint64_t elements, size_t size);

#endif