#include "synchronization/mutex.h"
#include "synchronization/spinlock.h"
#include "memory/kmalloc.h"
#include "system/scheduler.h"

#define ACQUIRE(mutex, current) (atomic_swap(&mutex->owner, current) == 0)

struct pcb_s;

mutex_t* mutex_init(){
    mutex_t* m = (mutex_t*) kmalloc(sizeof(mutex_t));
    if(!m) return NULL;
    m->owner = MUTEX_UNLOCKED;
    return m;
}

void mutex_acquire(mutex_t* mutex){
    pcb_t* current = get_current();

    DEBUG("Process %d attempting to acquire mutex at 0x%x...\n", current->pid, mutex);
    
    // otherwise, the mutex has been acquired already, we need to add it to the mutex's wait queue
    // enqueue a wait item for this mutex
    DEFINE_WAIT(waitqueue_entry);

    while(TRUE){
        if (ACQUIRE(mutex, (u64) current)){
            DEBUG("Mutex at 0x%x acquired.\n", mutex);
            return;
        }

        WARNING("Mutex has already been acquired. Waiting until release...\n");

        spinlock_acquire(&mutex->wait_lock);


        if(list_empty(&waitqueue_entry.entry)){
            list_add(&waitqueue_entry.entry, &mutex->wait_list.head);
        }

        current->state = PROCESS_BLOCKED;

        // release the spinlock
        spinlock_release(&mutex->wait_lock);

        // block
        deschedule();
    }
}

void mutex_release(mutex_t* mutex){
    // release the mutex
    atomic_store_release(&mutex->owner, MUTEX_UNLOCKED);
    INFO("Mutex released.\n");

    // look for an item in the wait queue to wake, if any
    spinlock_acquire(&mutex->wait_lock);

    if(!list_empty(&mutex->wait_list.head)){
        // pop single entry off the list
        wait_queue_entry_t* entry = list_entry(mutex->wait_list.head.next, wait_queue_entry_t, entry);

        // call the entry's wait function (in this case, the default one)
        entry->func(entry);

        // remove the entry from the list
        list_remove(&entry->entry);
    }

    spinlock_release(&mutex->wait_lock);

    // invoke the scheduler to allow the processes released by this mutex 
    // to acquire it before this process acquires it again
    scheduler();
}
