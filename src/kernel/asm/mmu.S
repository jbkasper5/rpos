#include "virtual_memory.h"

.globl mmu_init
mmu_init:
    stp x29, x30, [sp, #-16]!


    bl initialize_page_tables


    // TODO: L3 page table entry here
    // skip past first page table entry, make it blank
    add x0, x0, #PTE_SIZE

    // mark the page as valid
    orr x0, x0, 1               // valid
    orr x0, x0, (1 << 1)        // denotes this entry as a page descriptor
    orr x0, x0, (0b11 << 4)     // EL0 Read-only
    orr x0, x0, (0b11 << 6)     // inner-sharability
    orr x0, x0, (1 << 10)       // access flag, must be set to 1
    orr x0, x0, (0x80 << 12) // physical address that the page maps to 


    // put the page table base into the system register to denote page table 1
    msr TTBR0_EL1, x0

    mov x0, #(TCR_EL1_VALUE)
    msr TCR_EL1, x0

    ldr x0, =0xFF44FF
    msr MAIR_EL1, x0

    mrs x0, sctlr_el1
    orr x0, x0, #(1 << 0)      // M (MMU enable)
    msr sctlr_el1, x0

    ldp x29, x30, [sp], #16

    // data synchronization barrier inner-sharable
    dsb ish
    isb 

    ret
