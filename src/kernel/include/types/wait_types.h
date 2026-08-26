#ifndef __WAIT_TYPES_H__
#define __WAIT_TYPES_H__

#include "macros.h"
#include "types/spinlock_types.h"
#include "types/datastructures_types.h"

struct wait_queue_head {
	raw_spinlock_t		lock;
	list_head_t     	head;
};

struct wait_queue_entry;

typedef int (*wait_queue_func_t)(struct wait_queue_entry *wq_entry);

typedef struct wait_queue_entry {
    unsigned int        flags;
    void*               private;     // pointer back to the task_struct
    wait_queue_func_t   func;         // wake callback (default: try_to_wake_up)
    list_head_t         entry;        // the actual linkage
} wait_queue_entry_t;

typedef struct wait_queue_head wait_queue_head_t;

#endif
