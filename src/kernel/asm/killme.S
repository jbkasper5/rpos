#include "mm.h"

.section .text
.globl drop_to_user
drop_to_user:

    # set user stack
    ldr x0, =USTACK
    msr SP_EL0, x0

    adr x0, do_user_things // el1_entry points to the first instruction of
    msr ELR_EL1, x0 // EL0 code.

    mov x0, #0b00000 // DAIF=0000 M[4:0]=00000 EL0t.
    msr SPSR_EL1, x0
    eret