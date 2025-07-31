#include "mm.h"

.section ".text.boot"

# .globl _start

# .equ GPIO_BASE, 0xFE000000
# .equ GPFSEL2, 0x08
# .equ GPIO_21_OUTPUT, 0x8
# .equ GPFSET0, 0x1c
# .equ GPFCLx0, 0x28
# .equ GPIOVAL, 0x200000

# _start:
#     ldr x0, =GPIO_BASE
#     ldr x1, =GPIO_21_OUTPUT
#     str x1, [x0, #GPFSEL2]
#     ldr x2, =0x800000

# loop:
#     ldr x1, =GPIOVAL
#     str x1, [x0, #GPFSET0]

#     eor x10, x10, x10
#     delay1:
#         add x10, x10, #1
#         cmp x10, x2
#         bne delay1
    
#     ldr x1, =GPIOVAL
#     str x1, [x0, #GPFCLx0]

#     eor x10, x10, x10
#     delay2:
#         add x10, x10, #1
#         cmp x10, x2
#         bne delay2

#     b loop

_start: 
   mrs x0, mpidr_el1
   and x0, x0, #0xFF
   cbz x0, master
   b _infinite

master:
   adr x0, bss_begin
   adr x1, bss_end
   sub x1, x1, x0

   # clear the bss segment
   bl memzero

   mov sp, #LOW_MEMORY
   bl kernel_main
   b _infinite

_infinite:
    b _infinite
