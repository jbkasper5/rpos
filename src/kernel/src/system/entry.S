#include "system/entry.h"

.section .rodata
my_string:
    .asciz "ELR_EL1: 0x%x\n"
my_string_2:
    .asciz "Coming from EL: %d\n"
my_string_3:
    .asciz "IRQ from EL: %d\n"
pc_string:
    .asciz "Return PC: 0x%x\n"

.section .data
.align 3
.global user_context_ptr
user_context_ptr:
    .quad 0               // 8 bytes, initialized to 0

.section .text
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

.macro store_context
    adrp x0, user_context_ptr
    add  x0, x0, :lo12:user_context_ptr
    mov  x1, sp
    str  x1, [x0]
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

    // save SP_EL0 context
    mrs x0, SP_EL0
    stp x30, x0, [sp, #16 * 15]

    // THIS IS FINE
    mrs x0, ELR_EL1
    mrs x1, SPSR_EL1
    stp x0, x1, [sp, #16 * 16]

    // store user page table base into the stack as well
    mrs x0, TTBR0_EL1
    str x0, [sp, 16 * 17]

    #ifdef DEBUG
    mrs x1, ELR_EL1
    ldr x0, =pc_string
    bl printf
    #endif

    // reload x0 and x1 to preserve the parameters passed to the kernel
    ldp x0, x1, [sp, #16 * 0]

    # move stack pointer into x0, so when we call the scheduler, we're passing the register file to it
    # mov x0, sp
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

    // write value back into stack pointer
    msr SP_EL0, x0

    // restore user program counter and processor state
    ldp x0, x1, [sp, #16 * 16]
    msr ELR_EL1, x0
    msr SPSR_EL1, x1

    // restore user page table base from stack
    ldr x0, [sp, #16 * 17]
    msr TTBR0_EL1, x0

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
    ventry handle_el0_irq
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

# ERROR HANDLER FOR KERNEL MODE
sync_invalid_el1h:
    mrs x10, esr_el1
    mrs x11, far_el1
    mrs x12, elr_el1
    b .   // halt here for GDB to inspect
    # handle_invalid_entry SYNC_INVALID_EL1H

# IRQ HANDLER FOR KERNEL MODE
handle_el1_irq:
    kernel_entry

    #ifdef DEBUG
    mrs x1, SPSR_EL1
    lsr x1, x1, #2
    and x1, x1, #3         // x1 = EL (0, 1, etc.)

    ldr x0, =my_string_3
    bl printf
    #endif

    mrs x1, SPSR_EL1
    lsr x1, x1, #2
    and x1, x1, #3         // x1 = EL (0, 1, etc.)


    // don't handle IRQs for EL1->EL1 yet
    bl handle_irq
    kernel_exit
    eret


fiq_invalid_el1h:
    handle_invalid_entry FIQ_INVALUD_EL1H

error_invalid_el1h:
    handle_invalid_entry ERROR_INVALID_EL1H

# SYSTEM CALL HANDLER FOR USER MODE
handle_el0_svc:
    # disable interrupts while we're handling the system call
    msr daifset, #0xf

    // backup all user registers and processor state
    kernel_entry

    // store user context into context variable
    store_context

    #ifdef DEBUG
    mrs x1, SPSR_EL1
    lsr x1, x1, #2
    and x1, x1, #3         // x1 = EL (0, 1, etc.)

    ldr x0, =my_string_2
    bl printf
    #endif

    // prepare arguments for the system call
    ldp x0, x1, [sp, #16 * 0]
    ldp x2, x3, [sp, #16 * 1]
    ldp x4, x5, [sp, #16 * 2]    
    ldr x8, [sp, #16 * 4]

    // since x8 contains the syscall number, move it into parameter register 7 to call
    // the syscall handler
    mov x6, x8

    // handle the system call
    bl handle_syscall

    // restore registers and processor state
    kernel_exit
    eret


# IRQ HANDLER FOR USER MODE
handle_el0_irq:
    msr daifset, #0xf           // disable interrupts
    kernel_entry

    // store user context into context variable
    store_context

    #ifdef DEBUG
    mrs x1, SPSR_EL1
    lsr x1, x1, #2
    and x1, x1, #3         // x1 = EL (0, 1, etc.)

    ldr x0, =my_string_3
    bl printf
    #endif

    // put the register file pointer into x0
    mov x0, sp

    // reload EL into x1
    mrs x1, SPSR_EL1
    lsr x1, x1, #2
    and x1, x1, #3         // x1 = EL (0, 1, etc.)

    bl handle_irq
    kernel_exit
    eret



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
