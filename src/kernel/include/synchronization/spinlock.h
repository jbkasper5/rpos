#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "synchronization/atomic.h"

typedef struct raw_spinlock_s {
	volatile u32   lock;
} raw_spinlock_t;


#define SPIN_LOCK_UNLOCKED	{ 0 }

void spinlock_acquire(raw_spinlock_t* l);
void spinlock_release(raw_spinlock_t* l);

#endif