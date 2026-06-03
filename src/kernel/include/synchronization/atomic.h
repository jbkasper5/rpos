#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include "macros.h"

typedef volatile unsigned long atomic_long_t;

u64 atomic_swap(void* addr, u64 swap);

#endif
