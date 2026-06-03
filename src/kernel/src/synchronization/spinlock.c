#include "synchronization/spinlock.h"

void spinlock_acquire(raw_spinlock_t* l){
    return atomic_swap(l, 1);
}

void spinlock_release(raw_spinlock_t* l){
    return atomic_swap(l, 0);
}