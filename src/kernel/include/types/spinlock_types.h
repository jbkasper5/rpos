#ifndef __SPINLOCK_TYPES_H__
#define __SPINLOCK_TYPES_H__

#include "macros.h"

typedef struct raw_spinlock_s {
	volatile u32   lock;
} raw_spinlock_t;

#endif
