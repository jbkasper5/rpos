.section .text
.globl mmutest
mmutest:
    stp x29, x30, [sp, #-16]!   // Save frame pointer and link register
    mov x29, sp

    ldr x0, =0x0
    at s1e1r, x0
    isb
    mrs x1, par_el1
    mov x0, x1
    
    ldp x29, x30, [sp], #16     // Restore frame pointer and link register
    ret