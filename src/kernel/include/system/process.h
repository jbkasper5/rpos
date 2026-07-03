#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "macros.h"
#include "filesystem/filesystem.h"
#include "memory/kmalloc.h"
#include "filesystem/disk.h"

#define SET_PCB_REG_NUM(pcb, regnum, val)   (pcb->registers.regs[regnum] = val)
#define SET_PCB_REG(pcb, reg, val)          (pcb->registers.reg = val)

#define MAX_PROCESSES 64
#define MAX_OPEN_FILES 32

enum PROC_STATE{
    PROCESS_BLOCKED,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_TERMINATED,
};

enum WAIT_STATE{
    WAITING_ALL = -1,
    WAITING_NONE = 0
};

enum EXIT_STATUS{
    EX_OK           = 0,      /* successful termination */
    EX__BASE        = 64,      /* base value for error messages */
    EX_USAGE        = 64,      /* command line usage error */
    EX_DATAERR      = 65,      /* data format error */
    EX_NOINPUT      = 66,      /* cannot open input */    
    EX_NOUSER       = 67,      /* addressee unknown */    
    EX_NOHOST       = 68,      /* host name unknown */
    EX_UNAVAILABLE  = 69,      /* service unavailable */
    EX_SOFTWARE     = 70,      /* internal software error */
    EX_OSERR        = 71,      /* system error (e.g., can't fork) */
    EX_OSFILE       = 72,      /* critical OS file missing */
    EX_CANTCREAT    = 73,      /* can't create (user) output file */
    EX_IOERR        = 74,      /* input/output error */
    EX_TEMPFAIL     = 75,      /* temp failure; user is invited to retry */
    EX_PROTOCOL     = 76,      /* remote error in protocol */
    EX_NOPERM       = 77,      /* permission denied */
    EX_CONFIG       = 78,      /* configuration error */
};

typedef struct reglist_s{
    u64 regs[11];
    u64 sp;            // stack pointer to use on return
    u64 pc;            // instruction address to return to
    u64 spsr;          // processor state to return to
    u64 ttbr;          // page table base for the process
}reglist_t;

typedef struct pcb_s {
    reglist_t registers;
    u64 kernel_stack;

    enum PROC_STATE state;
    enum EXIT_STATUS exit_status;

    u32 pid;
    u32 waiting_on;

    struct pcb_s* parent;
    list_head_t children;
    list_head_t siblings;

    directory* cwd;

    file_t* fds[MAX_OPEN_FILES];
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

static inline pcb_t* get_current() {
    u64 pcb_addr;
    asm volatile("mrs %0, TPIDR_EL1" : "=r"(pcb_addr));
    return (pcb_t*)pcb_addr;
}

static inline void set_current(pcb_t* pcb) {
    asm volatile("msr TPIDR_EL1, %0" : : "r"(pcb));
}

pcb_t* procalloc(u64 entrypoint);
pcb_t* clone_active_proc();
int fd_alloc(pcb_t* proc, file_t* file);
u32 atomic_increment(u64 address, u32 increment);

#endif