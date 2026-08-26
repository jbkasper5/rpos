#ifndef __WAIT_H__
#define __WAIT_H__

#include "synchronization/spinlock.h"
#include "utils/datastructures.h"
#include "system/process.h"
#include "types/wait_types.h"

int default_wake_fn(wait_queue_entry_t* entry);

#define DEFINE_WAIT(name)                                          \
    wait_queue_entry_t name = {                                    \
        .private = get_current(),                                        \
        .func    = default_wake_fn,                                \
        .entry   = { &(name).entry, &(name).entry }                \
    }


#endif