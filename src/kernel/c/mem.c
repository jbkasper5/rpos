#include "mem.h"

void* memcpy(void* dest, const void* src, uint32_t bytes) {
    char* cdest = (char*)dest;
    const char* csrc = (const char*)src;
    for (uint32_t i = 0; i < bytes; i++) {
        cdest[i] = csrc[i];
    }
    return dest;
}