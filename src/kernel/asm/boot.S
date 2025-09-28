#include "mm.h"
#include "mmu.h"
#include "sysregs.h"

.section ".text.boot"

.globl _start
_start: 
   mrs x0, mpidr_el1
   and x0, x0, #0xFF
   cbz x0, master
   b _infinite

master:

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

   mov x0, #(1 << 0) | (1 << 10) | (1 << 11)   // NS | RW | ST
   msr SCR_EL3, x0

   ldr x0, =SCTLR_VALUE_MMU_DISABLED
   msr sctlr_el1, x0

   ldr x0, =HCR_VALUE
   msr hcr_el2, x0

   ldr x0, =SCR_VALUE
   msr scr_el3, x0

   ldr x0, =SPSR_VALUE
   msr spsr_el3, x0

   
   // –––––––– MMU INIT –––––––– //
   ldr x0, =CPACR_EL1_VAL
   msr CPACR_EL1, x0

   ldr x0, =TCR_EL1_VAL
   msr TCR_EL1, x0

   ldr x0, =MAIR_VALUE
   msr MAIR_EL1, x0
   // –––––––––––––––––––––––––– //

   adr x0, el1_entry
   msr elr_el3, x0

   eret

el1_entry:
   mov sp, #LOW_MEMORY
   adr x0, bss_begin
   adr x1, bss_end
   sub x1, x1, x0

   # clear the bss segment
   bl memzero

   bl kernel_main
   b _infinite

_infinite:
    b _infinite

.globl page_table_base
page_table_base:
   adrp x0, _page_table_base
   ret