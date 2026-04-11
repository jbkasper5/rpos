#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "macros.h"
#include "filesystem/filesystem.h"
#include "memory/kmalloc.h"

#define SET_PCB_REG_NUM(pcb, regnum, val)   (pcb->registers.regs[regnum] = val)
#define SET_PCB_REG(pcb, reg, val)          (pcb->registers.reg = val)

#define MAX_PROCESSES 64
#define MAX_OPEN_FILES 32

enum PROC_STATE{
    PROCESS_BLOCKED,
    PROCESS_READY,
    PROCESS_RUNNING,
};

typedef struct reglist_s{
    u64 regs[31];
    u64 sp;            // stack pointer to use on return
    u64 pc;            // instruction address to return to
    u64 spsr;          // processor state to return to
    u64 ttbr;          // page table base for the process
}reglist_t;

typedef struct PCB_S{
    enum PROC_STATE state;
    u32 pid;
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
    u32 processes;
} pcb_list_t;

extern pcb_list_t proclist;
extern u64 active_process;

pcb_t* procalloc();
pcb_t* clone_active_proc();

#endif