#include "synchronization/mutex.h"
#include "synchronization/spinlock.h"
#include "memory/kmalloc.h"


struct pcb_s;
static inline struct pcb_s* get_current() {
    u64 pcb_addr;
    asm volatile("mrs %0, TPIDR_EL1" : "=r"(pcb_addr));
    return (struct pcb_s*)pcb_addr;
}


mutex_t* mutex_init(){
    mutex_t* m = (mutex_t*) kmalloc(sizeof(mutex_t));
    if(!m) return NULL;
    m->owner = MUTEX_UNLOCKED;
    return m;
}

u64 mutex_acquire(mutex_t* mutex){
    spinlock_acquire(&mutex->wait_lock);

    void* curr = get_current();
    atomic_swap(&mutex->owner, curr);

    spinlock_release(&mutex->wait_lock);
}

u64 mutex_release(mutex_t* mutex){
    spinlock_acquire(&mutex->wait_lock);

    void* curr = get_current();
    atomic_swap(&mutex->owner, MUTEX_UNLOCKED);

    spinlock_release(&mutex->wait_lock);
}
