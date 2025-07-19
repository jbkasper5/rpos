
.section ".text.boot"

.global _start

_start:
    ldr     x2, =0x5
    mrs     x3, mpidr_el1
    and     x3, x3, #3              // get the process number
    cbz     x3, _core_zero_boot
    b       _infinite

1:  wfe
    b       1b


_core_zero_boot:
    ldr     x1, =_start             // set x1 to address 0x80000000
    mov     sp, x1                  // move that address into the stack pointer
    bl      main                    // jump to main
    b       _infinite

_infinite:
    b       _infinite