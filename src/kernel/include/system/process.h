#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "macros.h"
#include "filesystem/filesystem.h"
#include "memory/kmalloc.h"

#define MAX_PROCESSES 64
#define MAX_OPEN_FILES 32

enum PROC_STATE{
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
};

typedef struct reglist_s{
    uint64_t regs[31];
    uint64_t sp;            // stack pointer to use on return
    uint64_t pc;            // instruction address to return to
    uint64_t spsr;          // processor state to return to
    uint64_t ttbr;          // page table base for the process
}reglist_t;

typedef struct PCB_S{
    enum PROC_STATE state;
    uint32_t pid;
    reglist_t registers;
    file_t fds[MAX_OPEN_FILES];
    // memory information
        // page tables
    // file structure information
    // thread information
    // signal information
    // timer information
}pcb_t;

typedef struct PCB_LIST_S{
    pcb_t proclist[MAX_PROCESSES];
    uint32_t processes;
} pcb_list_t;

extern pcb_list_t proclist;
extern uint64_t active_process;

pcb_t* procalloc();

#endif