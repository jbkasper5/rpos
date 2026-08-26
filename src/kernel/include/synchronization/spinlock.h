#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "synchronization/atomic.h"
#include "types/spinlock_types.h"


#define SPIN_LOCK_UNLOCKED	{ 0 }

void spinlock_acquire(raw_spinlock_t* l);
void spinlock_release(raw_spinlock_t* l);

#endif