#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "macros.h"

#define MAX_PROCESSES 64

enum PROC_STATE{
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
};

typedef struct reglist_s{
    uint64_t regs[31];
    uint64_t sp;
    uint64_t pc;
    uint64_t spsr;
}reglist_t;

typedef struct PCB_S{
    enum PROC_STATE state;
    uint32_t pid;
    reglist_t registers;
    // memory information
    // register information
    // file structure information
    // file descriptor information
    // thread information
    // signal information
}pcb_t;

typedef struct PCB_LIST_S{
    pcb_t proclist[MAX_PROCESSES];
    uint32_t processes;
} pcb_list_t;

extern pcb_list_t proclist;
extern uint64_t active_process;

#endif