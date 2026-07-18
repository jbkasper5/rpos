#include "synchronization/wait.h"
#include "system/process.h"
#include "system/scheduler.h"

int default_wake_fn(wait_queue_entry_t* entry){
    pcb_t* task_to_wake = (pcb_t*) entry->private;
    reschedule(task_to_wake);
    return TRUE;
}