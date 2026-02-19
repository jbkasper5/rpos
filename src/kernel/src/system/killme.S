#include "memory/mm.h"

.section .text
.globl drop_to_user

// params: 
    // x0 -> user stack
    // x1 -> pc
    // x2 -> program state
    // x3 -> user ttbr
drop_to_user:
    msr TTBR0_EL1, x3

    dsb ish
    isb

    msr SP_EL0, x0
    msr ELR_EL1, x1 // EL0 code.
    msr SPSR_EL1, x2

    eret