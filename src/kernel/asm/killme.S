#include "mm.h"

.section .text
.globl drop_to_user
drop_to_user:

    # set user stack
    ldr x0, =USTACK
    msr SP_EL0, x0

    # set exception return address to go to user things
    ldr x0, =do_user_things
    msr ELR_EL1, x0

    # set SPSR
    mov x0, 0
    msr SPSR_EL1, x0

    # should be ready for eret now
    eret

    # # load the user stack into sp to test the memory is valid
    # ldr x0, =USTACK
    # bl _translate

    # ldr x0, =USTACK
    # mov sp, x0

    # ldr x30, =do_user_things
    # ret