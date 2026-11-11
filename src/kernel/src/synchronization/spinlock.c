#include "synchronization/spinlock.h"

void spinlock_init(raw_spinlock_t* l){
    l->lock = 0;
}

void spinlock_acquire(raw_spinlock_t* l){
    while(atomic_swap(&l->lock, SPINLOCK_LOCKED));
}

void spinlock_release(raw_spinlock_t* l){
    atomic_store_release((u32*) &l->lock, SPINLOCK_UNLOCKED);
}