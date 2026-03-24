#include "memory/mm.h"

.section .data
.align 3
.global device_tree_data
device_tree_data:
    .quad 0               // 8 bytes, initialized to 0

.section ".text.boot"
.globl _start
_start:
   // GPU provides the device tree info to x0 during boot, so save it off and continue
   // get device tree data (high map address)
   # ldr x1, =device_tree_data

   # // subtract virtual base to get phys address
   # ldr x2, =VIRT_BASE
   # sub x1, x1, x2

   # // store x0 into physical address of device tree ptr
   # str x0, [x1]

   // try to store the DTB address in x19
   mov x19, x0

   mrs x0, mpidr_el1
   and x0, x0, #0xFF
   cbz x0, master
   b _infinite

master:
   // initialize UART and set up debugging via JLINK
   ldr x0, =__kstack_top
   mov sp, x0

   bl irq_init_vectors

   // initialize UART
   bl uart_init_lo
   
   // initialize JLINK (if requested)
   #ifdef DEBUG
   bl debug_init_lo
   #endif

   # bl initialize_gic

   // initialize kernel stack for when we enter the trap handler
   ldr x0, =__kstack_top
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

   ldr x0, =el2_entry // el2_entry points to the first instruction of
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
   ldr x0, =map_high // very first job of EL1 is to set up the high map kernel
   msr ELR_EL2, x0

   mrs x0, cptr_el2
   bic x0, x0, #(1 << 10)
   msr cptr_el2, x0
   isb

   mrs x0, cptr_el2

   // specify the kernel to use SP_EL1
   msr spsel, #1

   // drop into el1
   eret

_infinite:
   b _infinite


# move this section to the highmap
.section .text
.globl el1_entry
el1_entry:
   ldr x0, =bss_begin
   ldr x1, =bss_end
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

   ldr x0, =__kstack_top_hi
   mov sp, x0

   bl kernel_main
