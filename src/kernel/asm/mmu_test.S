.section .text
.globl mmutest
mmutest:
    ldr x0, =0x0
    at s1e1r, x0
    isb
    mrs x1, par_el1
    mov x2, x1

    ldr x0, =0x1000
    at s1e1r, x0
    isb
    mrs x1, par_el1
