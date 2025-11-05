#include "scheduler_constants.h"

.section .text
.globl read_physical_timer
read_physical_timer:
    mrs x0, CNTPCT_EL0
    ret

.globl read_virtual_timer
read_virtual_timer:
    mrs x0, CNTVCT_EL0
    ret

.globl physical_timer_enable
physical_timer_enable:
    mov x0, #1
    msr CNTP_CTL_EL0, x0
    mov x0, #(1 << 1) | (1 << 0)  // EL0VCTEN | EL0PCTEN
    msr cntkctl_el1, x0
    ret

// TVAL uses relative delay
.globl prime_physical_timer
prime_physical_timer:
    ldr x0, =QUANTUM
    msr CNTP_TVAL_EL0, x0
    ret

.globl virtual_timer_enable
virtual_timer_enable:
    mov x0, #(1 << 1) | (1 << 0)  // EL0VCTEN | EL0PCTEN
    msr cntkctl_el1, x0 
    ret

// CVAL uses absolute delay
.globl prime_virtual_timer
prime_virtual_timer:
    msr CNTV_CVAL_EL0, x0
    mov x0, #1 // enable, no imask, no status
    msr CNTV_CTL_EL0, x0
    ret

.globl clear_virtual_timer
clear_virtual_timer:
    mov x0, #0 // disable, no imask, no status
    msr CNTV_CTL_EL0, x0
    ret