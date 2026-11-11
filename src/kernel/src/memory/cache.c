#include "memory/cache.h"

/*
    Some context for asm volatile:
        asm volatile ( "template" : outputs : inputs : clobbers );
*/

void dcache_clean_range(u64 start, u64 end){
    for(u64 a = ALIGN_DOWN(start, 64); a < end; a += 64)
        asm volatile("dc cvac, %0" :: "r"(a) : "memory");
    asm volatile("dsb sy" ::: "memory");
}

void dcache_clean_invalidate_range(u64 start, u64 end){
    for(u64 a = ALIGN_DOWN(start, 64); a < end; a += 64)
        asm volatile("dc civac, %0" :: "r"(a) : "memory");
    asm volatile("dsb sy" ::: "memory");
}

void dcache_invalidate_range(u64 start, u64 end){
    for(u64 a = ALIGN_DOWN(start, 64); a < end; a += 64)
        asm volatile("dc ivac, %0" :: "r"(a) : "memory");
    asm volatile("dsb sy" ::: "memory");
}