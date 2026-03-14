// for the sake of ease, this syscall wrapper will take:
//      x0 as syscall number
//      x1-x6 arguments
// then adjust accordingly
syscall:
    stp x29, x30, [sp, #-16]!
    mov x8, x0
    mov x0, x1
    mov x1, x2
    mov x2, x3
    mov x3, x4
    mov x4, x5
    mov x5, x6
    svc #55
    ldp x29, x30, [sp], #16
    ret