#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "system/scheduler_constants.h"
#include "system/process.h"
#include "macros.h"

extern void drop_to_user(u64 sp, u64 pc, u64 spsr, u64 ttbr, u64 kstack);

void scheduler_init();
void start_scheduler();
void add_to_schedule(pcb_t* proc);
void scheduler();
void deschedule();
void reschedule(u64 procnum);

void add_test_section_to_scheduler();

extern void context_switch();

#endif
