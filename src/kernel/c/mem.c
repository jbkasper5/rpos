#include "mem.h"

void* memcpy(void* dest, const void* src, uint32_t bytes) {
    char* cdest = (char*)dest;
    const char* csrc = (const char*)src;
    for (uint32_t i = 0; i < bytes; i++) {
        cdest[i] = csrc[i];
    }
    return dest;
}

void* memset(void* ptr, uint64_t bytes, int8_t value){
    unsigned char* addr = (unsigned char*) ptr;
    for(uint64_t i = 0; i < bytes; i++) addr[i] = value;
    return ptr;
}
