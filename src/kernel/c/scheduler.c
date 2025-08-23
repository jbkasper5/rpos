#include "scheduler.h"
#include "process.h"

void scheduler_init(){
    // for now, we say there is 1 task, the IDLE task, which is a placeholder for when the CPU is idle
    proclist.processes = 1;
}

void scheduler(){

}