#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "synchronization/atomic.h"
#include "types/spinlock_types.h"
#include "types/scalar.h"


#define SPIN_LOCK_UNLOCKED	{ 0 }

typedef enum {
    SPINLOCK_UNLOCKED = 0,
    SPINLOCK_LOCKED = 1,
} spilock_states;

void spinlock_init(raw_spinlock_t* l);
void spinlock_acquire(raw_spinlock_t* l);
void spinlock_release(raw_spinlock_t* l);

#endif