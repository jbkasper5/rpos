#include "scheduler_constants.h"

.globl read_timer
read_timer:
    mrs x0, CNTP_TVAL_EL0
    ret

.globl physical_timer_enable
physical_timer_enable:
    mov x0, #1
    msr CNTP_CTL_EL0, x0
    ret

.globl prime_physical_timer
prime_physical_timer:
    ldr x0, =QUANTUM
    msr CNTP_TVAL_EL0, x0
    ret
