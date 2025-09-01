#include "entry.h"

.section .rodata
my_string:
    .asciz "ELR_EL1: 0x%x\n"
my_string_2:
    .asciz "EL: %d\n"

.macro ventry label
.align 7
    b   \label
.endm

.macro handle_invalid_entry type
    kernel_entry
    mov x0, #\type
    mrs x1, esr_el1
    mrs x2, elr_el1
    bl show_invalid_entry_message
    b err_hang
.endm


.macro kernel_entry
    sub sp, sp, #S_FRAME_SIZE
    stp x0, x1, [sp, #16 * 0]
    stp x2, x3, [sp, #16 * 1]
    stp x4, x5, [sp, #16 * 2]
    stp x6, x7, [sp, #16 * 3]
    stp x8, x9, [sp, #16 * 4]
    stp x10, x11, [sp, #16 * 5]
    stp x12, x13, [sp, #16 * 6]
    stp x14, x15, [sp, #16 * 7]
    stp x16, x17, [sp, #16 * 8]
    stp x18, x19, [sp, #16 * 9]
    stp x20, x21, [sp, #16 * 10]
    stp x22, x23, [sp, #16 * 11]
    stp x24, x25, [sp, #16 * 12]
    stp x26, x27, [sp, #16 * 13]
    stp x28, x29, [sp, #16 * 14]
    mrs x0, SP_EL0
    stp x30, x0, [sp, #16 * 15]
    mrs x0, ELR_EL1
    mrs x1, SPSR_EL1
    stp x0, x1, [sp, #16 * 16]

    # move stack pointer into x0, so when we call the scheduler, we're passing the register file to it
    mov x0, sp
.endm

.macro kernel_exit
    ldp x2, x3, [sp, #16 * 1]
    ldp x4, x5, [sp, #16 * 2]
    ldp x6, x7, [sp, #16 * 3]
    ldp x8, x9, [sp, #16 * 4]
    ldp x10, x11, [sp, #16 * 5]
    ldp x12, x13, [sp, #16 * 6]
    ldp x14, x15, [sp, #16 * 7]
    ldp x16, x17, [sp, #16 * 8]
    ldp x18, x19, [sp, #16 * 9]
    ldp x20, x21, [sp, #16 * 10]
    ldp x22, x23, [sp, #16 * 11]
    ldp x24, x25, [sp, #16 * 12]
    ldp x26, x27, [sp, #16 * 13]
    ldp x28, x29, [sp, #16 * 14]
    ldp x30, x0, [sp, #16 * 15]

    msr SP_EL0, x0
    ldp x0, x1, [sp, #16 * 16]
    msr ELR_EL1, x0
    msr SPSR_EL1, x1


    mov x1, x0
    ldr x0, =my_string
    bl printf


    ldp x0, x1, [sp, #16 * 0]
    add sp, sp, #S_FRAME_SIZE
.endm

.align 11
.globl vectors
vectors:
    ventry sync_invalid_el1t
    ventry irq_invalid_el1t
    ventry fiq_invalid_el1t
    ventry error_invalid_el1t

    ventry sync_invalid_el1h
    ventry handle_el1_irq
    ventry fiq_invalid_el1h
    ventry error_invalid_el1h

    # system calls from user processes
    ventry handle_el0_svc
    ventry irq_invalid_el0_64
    ventry fiq_invalid_el0_64
    ventry error_invalid_el0_64

    ventry sync_invalid_el0_32
    ventry irq_invalid_el0_32
    ventry fiq_invalid_el0_32
    ventry error_invalid_el0_32


sync_invalid_el1t:
    handle_invalid_entry SYNC_INVALID_EL1T

irq_invalid_el1t:
    handle_invalid_entry IRQ_INVALID_EL1T

fiq_invalid_el1t:
    handle_invalid_entry FIQ_INVALUD_EL1T

error_invalid_el1t:
    handle_invalid_entry ERROR_INVALID_EL1T

sync_invalid_el1h:
    handle_invalid_entry SYNC_INVALID_EL1H

handle_el1_irq:
    kernel_entry
    bl handle_irq
    kernel_exit
    eret

fiq_invalid_el1h:
    handle_invalid_entry FIQ_INVALUD_EL1H

error_invalid_el1h:
    handle_invalid_entry ERROR_INVALID_EL1H

handle_el0_svc:
    # disable interrupts while we're handling the system call
    msr daifset, #0xf
    kernel_entry
    bl handle_syscall
    kernel_exit

    # re-enable interrupts after system call is completed
    msr daifclr, #0xf
    eret

irq_invalid_el0_64:
    handle_invalid_entry IRQ_INVALID_EL0_64

fiq_invalid_el0_64:
    handle_invalid_entry FIQ_INVALUD_EL0_64

error_invalid_el0_64:
    handle_invalid_entry ERROR_INVALID_EL0_64

sync_invalid_el0_32:
    handle_invalid_entry SYNC_INVALID_EL0_32

irq_invalid_el0_32:
    handle_invalid_entry IRQ_INVALID_EL0_32

fiq_invalid_el0_32:
    handle_invalid_entry FIQ_INVALUD_EL0_32

error_invalid_el0_32:
    handle_invalid_entry ERROR_INVALID_EL0_32


.globl err_hang
err_hang:
    b err_hang
