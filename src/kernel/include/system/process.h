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
    reglist_t registers;
    u64 kernel_stack;
    u32 pid;
    enum PROC_STATE state;
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

static inline pcb_t* get_current() {
    u64 pcb_addr;
    asm volatile("mrs %0, TPIDR_EL1" : "=r"(pcb_addr));
    return (pcb_t*)pcb_addr;
}

static inline void set_current(pcb_t* pcb) {
    asm volatile("msr TPIDR_EL1, %0" : : "r"(pcb));
}

pcb_t* procalloc();
pcb_t* clone_active_proc();
u32 atomic_increment(u64 address, u32 increment);

#endif