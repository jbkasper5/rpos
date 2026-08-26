#include "synchronization/spinlock.h"


void spinlock_acquire(raw_spinlock_t* l){
    while(atomic_swap(&l->lock, 1));
}

void spinlock_release(raw_spinlock_t* l){
    atomic_store_release((u64*) &l->lock, 0);
}