.section .data
page_table:
    .space 4096         // reserve 4096 bytes (4KB) for your page table

.globl mmu_init
mmu_init:
    ret
