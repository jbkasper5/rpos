#include "mem.h"
#include "macros.h"

void _neon_64_memcpy();
void _neon_16_memcpy();
void _neon_8_memset();
void _neon_16_memset();
void _neon_32_memset();
void _neon_64_memset();

void* memcpy(void* dest, const void* src, uint32_t bytes) {
    void* original_dest = dest;
    uint64_t copy_iters = bytes / 64;
    _neon_64_memcpy(dest, src, copy_iters);

    dest = UNSCALED_POINTER_ADD(dest, copy_iters * 64);
    src = UNSCALED_POINTER_ADD(src, copy_iters * 64);

    uint64_t remaining_bytes = bytes % 64;
    copy_iters = remaining_bytes / 16;
    _neon_16_memcpy(dest, src, copy_iters);

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

void* memset(void* ptr, uint64_t value, uint64_t elements, size_t size){
    uint64_t granule = 16 / size;
    uint64_t tail = elements % granule;
    uint64_t vectors  = elements / granule;

    if(vectors){
        switch(size){
            case 1: _neon_8_memset(ptr, value, vectors); break;
            case 2: _neon_16_memset(ptr, value, vectors); break;
            case 4: _neon_32_memset(ptr, value, vectors); break;
            case 8: _neon_64_memset(ptr, value, vectors); break;
        }
    }

    if(tail){
        uint8_t* ptr8 = (uint8_t*)ptr + vectors * 16; // advance pointer past NEON vectors
        for(uint64_t i = 0; i < tail; i++){
            switch(size){
                case 1: ptr8[i] = (uint8_t)value; break;
                case 2: ((uint16_t*)ptr8)[i] = (uint16_t)value; break;
                case 4: ((uint32_t*)ptr8)[i] = (uint32_t)value; break;
                case 8: ((uint64_t*)ptr8)[i] = (uint64_t)value; break;
            }
        }
    }

    return ptr;
}
