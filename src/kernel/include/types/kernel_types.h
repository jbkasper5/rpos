#ifndef __KERNEL_TYPES_H__
#define __KERNEL_TYPES_H__

#include "types/scalar.h"
typedef struct trap_frame_s {
    u64 regs[31];    // [0..30]  x0..x30   (regs[29]=fp/x29, regs[30]=lr/x30)
    u64 sp_el0;      // [31]     stp x30, x0, [sp, #16*15]  -> hi half
    u64 elr_el1;     // [32]     stp x0,  x1, [sp, #16*16]  -> lo half
    u64 spsr_el1;    // [33]                                -> hi half
} trap_frame_t;

#endif
