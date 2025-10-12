#include "mm.h"
#include "sysregs.h"
.section ".text.boot"

.globl _start
_start: 
   mrs x0, mpidr_el1
   and x0, x0, #0xFF
   cbz x0, test_master
   b _infinite



test_master:
   mov sp, #KSTACK
   bl uart_init
   bl debug_init
   bl irq_init_vectors

   // EL3 confirmed
   bl get_el

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

   eret

el2_entry:
   // EL2 confirmed
   bl get_el
   msr SCTLR_EL1, xzr 
   mrs x0, HCR_EL2
   ORR x0, x0, #(1<<31) // RW=1 EL1 Execution state is AArch64.
   msr HCR_EL2, x0
   mov x0, #0b00101 // DAIF=0000
   msr SPSR_EL2, x0 // M[4:0]=00101 EL1h must match HCR_EL2.RW.
   adr x0, test_el1_entry // el1_entry points to the first instruction of
   msr ELR_EL2, x0 // EL1 code.
   eret

test_el1_entry:
   // EL1 confirmed
   bl get_el
   mov x0, #0b00000 // DAIF=0000 M[4:0]=00000 EL0t.
   msr SPSR_EL1, x0
   adr x0, el0_entry // el1_entry points to the first instruction of
   msr ELR_EL1, x0 // EL0 code.
   eret

el0_entry:
   ldr x30, =do_user_things_asm
   ret

master:
   // initialize UART and set up debugging via JLINK
   mov sp, #KSTACK
   bl uart_init
   bl debug_init


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

   adr x0, el1_entry
   msr elr_el3, x0

   eret

el1_entry:
   adr x0, bss_begin
   adr x1, bss_end
   sub x1, x1, x0

   # clear the bss segment
   bl memzero

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

