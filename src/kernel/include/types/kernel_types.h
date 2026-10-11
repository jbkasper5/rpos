#ifndef __KERNEL_TYPES_H__
#define __KERNEL_TYPES_H__

#include "types/scalar.h"
typedef struct trap_frame_s {
    u64 regs[31];    // [0..30]  x0..x30   (regs[29]=fp/x29, regs[30]=lr/x30)
    u64 sp_el0;      // [31]     stp x30, x0, [sp, #16*15]  -> hi half
    u64 elr_el1;     // [32]     stp x0,  x1, [sp, #16*16]  -> lo half
    u64 spsr_el1;    // [33]                                -> hi half
} trap_frame_t;

typedef struct cs_stack_frame_s {
    u64 x29;   /* frame pointer                        +0x00 */
    u64 x30;   /* link register -- where the thread resumes  +0x08 */
    u64 x27;   /*                                      +0x10 */
    u64 x28;
    u64 x25;
    u64 x26;
    u64 x23;
    u64 x24;
    u64 x21;
    u64 x22;
    u64 x19;
    u64 x20;   /*                                      +0x58 */
} cs_stack_frame_t;


#define BASE_TRAPFRAME(kstack)              ((trap_frame_t*)(((u64) ALIGN_UP(kstack, PAGE_SIZE)) - 0x10 - sizeof(trap_frame_t)))

#endif
