#include "memory/mm.h"

.section .text
.globl drop_to_user

// params: 
    // x0 -> user stack
    // x1 -> pc
    // x2 -> program state
drop_to_user:

    # ldr x3, =USTACK
    # AT S1E0R, x3
    # mrs x1, PAR_EL1     // read result

    msr SP_EL0, x0
    msr ELR_EL1, x1 // EL0 code.
    msr SPSR_EL1, x2
    eret