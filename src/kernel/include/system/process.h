#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "macros.h"
#include "filesystem/filesystem.h"
#include "memory/kmalloc.h"
#include "filesystem/disk.h"
#include "types/process_types.h"

#define SET_PCB_REG_NUM(pcb, regnum, val)   (pcb->registers.regs[regnum] = val)
#define SET_PCB_REG(pcb, reg, val)          (pcb->registers.reg = val)

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
