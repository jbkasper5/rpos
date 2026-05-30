#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "macros.h"

typedef struct raw_spinlock_s {
	volatile u32   lock;
} raw_spinlock_t;


#define SPIN_LOCK_UNLOCKED	{ 0 }

#endif