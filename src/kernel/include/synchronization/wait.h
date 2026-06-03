#ifndef __WAIT_H__
#define __WAIT_H__

#include "synchronization/spinlock.h"
#include "utils/datastructures.h"
#include "system/process.h"

struct wait_queue_head {
	raw_spinlock_t		lock;
	list_head_t     	head;
};

typedef int (*wait_queue_func_t)(struct wait_queue_entry *wq_entry);

typedef struct wait_queue_entry {
    unsigned int        flags;
    void                *private;     // pointer back to the task_struct
    wait_queue_func_t   func;         // wake callback (default: try_to_wake_up)
    list_head_t         entry;        // the actual linkage
} wait_queue_entry_t;


int default_wake_fn(wait_queue_entry_t* entry);

#define DEFINE_WAIT(name)                                          \
    wait_queue_entry_t name = {                                    \
        .private = get_current(),                                        \
        .func    = default_wake_fn,                                \
        .entry   = { &(name).entry, &(name).entry }                \
    }


typedef struct wait_queue_head wait_queue_head_t;


#endif