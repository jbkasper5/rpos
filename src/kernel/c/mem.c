#include "mem.h"

void* memcpy(void* dest, const void* src, uint32_t n) {
    //simple implementation...
    uint8_t *bdest = (uint8_t *)dest;
    uint8_t *bsrc = (uint8_t *)src;

    for (int i=0; i<n; i++) {
        bdest[i] = bsrc[i];
    }

    return dest;
}