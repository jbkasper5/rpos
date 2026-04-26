.section .text

// void context_switch(u64* new_kernel_stack, u64* old_kernel_stack_save_point)
.globl context_switch
context_switch:

    // push callee-saved registers
    stp x19, x20, [sp, #-16]! // subtract 16 from sp, store 19 at offset 0, store 20 at offset 8
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x28, [sp, #-16]!
    stp x29, x30, [sp, #-16]!

    // save the current stack pointer into the buffer argument
    mov x19, sp
    str x19, [x1]

    // load the new context stack pointer
    mov sp, x0

    ldp x29, x30, [sp], #16
    ldp x27, x28, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16

    ret