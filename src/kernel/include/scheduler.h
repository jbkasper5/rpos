#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "scheduler_constants.h"
#include "process.h"
#include "macros.h"

extern void drop_to_user(uint64_t sp, uint64_t pc, uint64_t spsr);

void scheduler_init();
void start_scheduler();
void scheduler(reglist_t* reg_addr);
void deschedule();
void reschedule(uint64_t procnum);

#endif
