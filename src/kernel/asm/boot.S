#include "mm.h"
.section ".text.boot"

.globl _start
_start: 
   mrs x0, mpidr_el1
   and x0, x0, #0xFF
   cbz x0, master
   b _infinite



# test_master:
#    mov sp, #KSTACK
#    bl uart_init

#    #ifdef DEBUG
#    bl debug_init
#    #endif

#    bl irq_init_vectors
   
#    // set up kernel stack when we trap into exception handler
#    mov x0, #KSTACK
#    msr SP_EL1, x0

#    msr SCTLR_EL2, xzr
#    msr HCR_EL2, xzr

#    mrs x0, SCR_EL3
#    orr x0, x0, #(1<<10) // RW EL2 Execution state is AArch64.
#    orr x0, x0, #(1<<0) // NS EL1 is Non-secure world.
#    msr SCR_EL3, x0
#    mov x0, #0b01001 // DAIF=0000
#    msr SPSR_EL3, x0 // M[4:0]=01001 EL2h must match SCR_EL3.RW

#    adr x0, el2_entry // el2_entry points to the first instruction of
#    msr ELR_EL3, x0 

#    eret

# test_el1_entry:
#    // EL1 confirmed
#    bl get_el
#    mov x0, #0b00000 // DAIF=0000 M[4:0]=00000 EL0t.
#    msr SPSR_EL1, x0

#    // set up user stack
#    mov x0, #USTACK
#    msr SP_EL0, x0

#    adr x0, el0_entry // el1_entry points to the first instruction of
#    msr ELR_EL1, x0 // EL0 code.
#    eret

# el0_entry:
#    b do_user_things

master:
   // initialize UART and set up debugging via JLINK
   mov sp, #KSTACK
   bl uart_init
   
   #ifdef DEBUG
   bl debug_init
   #endif

   bl irq_init_vectors

   // initialize kernel stack for when we enter the trap handler
   mov x0, #KSTACK
   msr SP_EL1, x0

   // tell kernel to use SP_EL1 instead of SP_EL0
   # msr SPSel, #1

   // Enable GIC Distributor
   ldr x0, =0xFF840000           // GICD base
   mov w1, #1
   str w1, [x0, #0x000]          // GICD_CTLR = 1

   // Enable CPU Interface
   ldr x0, =0xFF842000           // GICC base
   mov w1, #1
   str w1, [x0, #0x000]          // GICC_CTLR = 1
   mov w1, #0xFF
   str w1, [x0, #0x004]          // GICC_PMR = 0xFF (priority mask)

   // zero out reigsters
   msr SCTLR_EL2, xzr
   msr HCR_EL2, xzr

   mrs x0, SCR_EL3
   orr x0, x0, #(1<<10) // RW EL2 Execution state is AArch64.
   orr x0, x0, #(1<<0) // NS EL1 is Non-secure world.
   msr SCR_EL3, x0
   mov x0, #0b01001 // DAIF=0000
   msr SPSR_EL3, x0 // M[4:0]=01001 EL2h must match SCR_EL3.RW

   adr x0, el2_entry // el2_entry points to the first instruction of
   msr ELR_EL3, x0 

   // drop into EL2
   eret

el2_entry:
   mov x0, #(1 << 29) | (1 << 28) | (1 << 23) | (1 << 22)
   msr SCTLR_EL1, x0
   isb
   mrs x0, HCR_EL2
   ORR x0, x0, #(1<<31) // RW=1 EL1 Execution state is AArch64.
   msr HCR_EL2, x0
   mov x0, #0b00101 // DAIF=0000
   msr SPSR_EL2, x0 // M[4:0]=00101 EL1h must match HCR_EL2.RW.
   adr x0, el1_entry // el1_entry points to the first instruction of
   msr ELR_EL2, x0 // EL1 code.

   // specify the kernel to use SP_EL1
   msr spsel, #1

   // drop into el1
   eret

el1_entry:
   adr x0, bss_begin
   adr x1, bss_end
   sub x1, x1, x0

   # clear the bss segment
   bl memzero

   mov x0, #0b00000 // DAIF=0000 M[4:0]=00000 EL0t.
   msr SPSR_EL1, x0

   mov sp, #KSTACK
   bl kernel_main
   b _infinite

_infinite:
    b _infinite


.globl get_bss_begin
get_bss_begin:
   adr x0, bss_begin
   ret

.globl get_bss_end
get_bss_end:
   adr x0, bss_end
   ret

