#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include "macros.h"
#include "types/atomic_types.h"

static inline void atomic_store_release(volatile u64 *p, u64 val) {
    __asm__ volatile("stlr %x1, [%0]" :: "r"(p), "r"(val) : "memory");
}

u64 atomic_swap(void* addr, u64 swap);

#endif
