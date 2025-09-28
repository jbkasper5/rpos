.section .data
.align 2
debug_str:
    .asciz "HERE!\n"

.section .text
.equ TCR_EL1_VALUE, \
    ((16 << 0)   | /* T0SZ = 48-bit VA space */ \
     (0b01 << 8) | /* IRGN0 = Write-back, Write-allocate */ \
     (0b01 << 10)| /* ORGN0 = Write-back, Write-allocate */ \
     (0b11 << 12)| /* SH0 = Inner Shareable */ \
     (0b00 << 14)| /* TG0 = 4KB granule */ \
     (16 << 16)  | /* T1SZ = 48-bit VA space */ \
     (0b01 << 24)| /* IRGN1 */ \
     (0b01 << 26)| /* ORGN1 */ \
     (0b11 << 28)| /* SH1 = Inner Shareable */ \
     (0b10 << 30)) /* TG1 = 4KB granule */

.globl mmu_init
mmu_init:
    stp x29, x30, [sp, #-16]!

    ldr x0, =_page_table_base
    ldr x1, =_page_table_metadata
    // c code to initialize the page tables
    bl initialize_page_tables


    // –––––––––––––––––––– NO VM FOR NOW –––––––––––––––––––– //

    # // HERE!
    ldr x0, =debug_str
    bl printf


    ldr x0, =_page_table_base
    // put the page table base into the system register to denote page table 1
    // for now, set both kernel and user page tables to the same location
    msr TTBR0_EL1, x0
    msr TTBR1_EL1, x0

    ldr x0, =TCR_EL1_VALUE
    msr TCR_EL1, x0

    // HERE!
    ldr x0, =debug_str
    bl printf

    isb

    ldr x0, =0xFF44FF
    msr MAIR_EL1, x0

    // HERE!
    ldr x0, =debug_str
    bl printf

    dsb ish
    isb

    // HERE!
    ldr x0, =debug_str
    bl printf

    mrs x0, sctlr_el1
    orr x0, x0, #(1 << 0)      // M (MMU enable)
    msr sctlr_el1, x0

    isb
    // ––––––––––––––––––––––––––––––––––––––––––––––––––––––– //

    // HERE!
    ldr x0, =debug_str
    bl printf

    ldp x29, x30, [sp], #16
    ret
