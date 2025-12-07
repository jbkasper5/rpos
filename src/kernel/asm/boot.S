#include "mm.h"
.section ".text.boot"

.globl _start
_start: 
   mrs x0, mpidr_el1
   and x0, x0, #0xFF
   cbz x0, master
   b _infinite

master:
   // initialize UART and set up debugging via JLINK
   adrp x0, __kstack_top
   mov sp, x0
   bl uart_init
   
   #ifdef DEBUG
   bl debug_init
   #endif

   bl irq_init_vectors

   // initialize kernel stack for when we enter the trap handler
   adrp x0, __kstack_top
   mov sp, x0
   msr SP_EL1, x0

   mrs x0, CPTR_EL3
   bic x0, x0, #(3 << 10)      // FPEN bits -> allow FP at EL2/EL1
   msr CPTR_EL3, x0

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

   mrs x0, cptr_el2
   bic x0, x0, #(1 << 10)
   msr cptr_el2, x0
   isb

   mrs x0, cptr_el2

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

   // At EL1, MMU off
   mrs x0, cpacr_el1
   orr x0, x0, #(3<<20)    // enable FP/NEON
   msr cpacr_el1, x0
   isb

   mrs x0, cpacr_el1

   mov w1, #0xBEEF
   dup v0.16b, w1

   adrp x0, __kstack_top
   mov sp, x0

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

