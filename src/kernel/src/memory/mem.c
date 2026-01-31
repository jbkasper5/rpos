#include "memory/mem.h"
#include "macros.h"

void _neon_64_memcpy();
void _neon_16_memcpy();
void _neon_8_memset();

void* memcpy(void* dest, const void* src, uint32_t bytes) {
    void* original_dest = dest;
    uint64_t copy_iters = bytes / 64;

    if(copy_iters) _neon_64_memcpy(dest, src, copy_iters);

    dest = UNSCALED_POINTER_ADD(dest, copy_iters * 64);
    src = UNSCALED_POINTER_ADD(src, copy_iters * 64);

    uint64_t remaining_bytes = bytes % 64;
    copy_iters = remaining_bytes / 16;
    if (copy_iters) _neon_16_memcpy(dest, src, copy_iters);

    dest = UNSCALED_POINTER_ADD(dest, copy_iters * 16);
    src = UNSCALED_POINTER_ADD(src, copy_iters * 16);

    // copy remaining bytes by char here
    remaining_bytes %= 16;
    const char* csrc = (const char*)src;
    char* cdest = (char*)dest;
    for (uint32_t i = 0; i < remaining_bytes; i++) {
        cdest[i] = csrc[i];
    }
    return original_dest;
}

void* memset(void* ptr, int value, size_t len){
    void* orig = ptr;
    size_t tail = len % 16;
    size_t vectors  = len / 16;

    _neon_8_memset(ptr, (unsigned char)value, vectors);

    ptr = UNSCALED_POINTER_ADD(orig, vectors * 16);

    for(size_t i = 0; i < tail; i++){
        ((unsigned char*)ptr)[i] = (unsigned char) value;
    }

    return orig;
}
