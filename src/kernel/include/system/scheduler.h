#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "system/scheduler_constants.h"
#include "system/process.h"
#include "macros.h"

extern void drop_to_user(u64 kstack, u64 ttbr);

void scheduler_init();
void start_scheduler();
void add_to_schedule(pcb_t* proc);
void scheduler();
void deschedule();
void reschedule(pcb_t* proc);

void add_test_section_to_scheduler();
void add_shell_to_scheduler();

extern void context_switch();

#endif
