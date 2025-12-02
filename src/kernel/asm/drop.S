#include "mm.h"

.section .text
.globl drop_to_user

// params: 
    // x0 -> user stack
    // x1 -> pc
    // x2 -> program state
drop_to_user:
    msr SP_EL0, x0
    msr ELR_EL1, x1 // EL0 code.
    msr SPSR_EL1, x2
    eret