#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "scheduler_constants.h"
#include "process.h"
#include "macros.h"

void scheduler_init();
void scheduler(reglist_t* reg_addr);

#endif
