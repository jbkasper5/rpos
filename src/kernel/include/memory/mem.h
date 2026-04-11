#ifndef __MEM_H__
#define __MEM_H__

#include "macros.h"

void* memcpy(void* dest, const void* src, u32 bytes);
void* memset(void* ptr, int value, size_t len);

#endif