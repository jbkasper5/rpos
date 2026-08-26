#include "memory/mm.h"

.section .text
.globl drop_to_user

// params: 
    // x0 -> user stack
    // x1 -> pc
    // x2 -> program state
    // x3 -> user ttbr
    // x4 -> new kstack
drop_to_user:
    // surgical clear of TLB mappings for specific ASID
    # mov x5, #0
    # tlbi aside1is, x5
    # dsb ish
    # isb

    // switch out new table
    msr TTBR0_EL1, x3
    dsb ish
    isb


    // nuke to fully wipe the TLB
    tlbi vmalle1is
    dsb ish
    isb

    msr SP_EL0, x0
    msr ELR_EL1, x1 // EL0 code.
    msr SPSR_EL1, x2
    mov sp, x4

    eret