.section ".text.boot"

.globl enable_mmu
enable_mmu:
    // 1. Setup MAIR (Index 0 = Normal WB/WA/RA)
    ldr x2, =((0x44 << 0) | (0x04 << 8) | (0xFF << 16))
    msr MAIR_EL1, x2

    // 2. Setup TCR (40-bit IPS, 4KB Granules, Inner-Shareable WB/WA)
    # ldr x2, =((16 << 0)  | (16 << 16) | (2 << 30) | (3 << 12) | \
    #           (3 << 28)  | (1 << 8)   | (1 << 10) | (1 << 24) | \
    #           (1 << 26)  | (2 << 32))
    ldr x2, =0x80100010
    # ldr x2, =0xB5103510
    msr TCR_EL1, x2
    isb

    // 3. Set Tables
    msr TTBR1_EL1, x0
    msr TTBR0_EL1, x1

    mov x4, x0
    mov x5, x1
    isb

    // 4. Invalidate I-Cache to be safe
    ic ialluis
    dsb ish
    isb

    // 4.5. Invalidate the dcache
    bl __asm_invalidate_dcache_all

    mov x0, #0
    mov x1, #0x1000000     // low 16 MiB: covers kernel, page-frame array, page tables
    bl  __asm_invalidate_dcache_range

    // 5. Enable MMU
    mrs x2, sctlr_el1
    orr x2, x2, #(1 << 0)    // M
    orr x2, x2, #(1 << 2)    // C
    # orr x2, x2, #(1 << 12)   // I
    msr sctlr_el1, x2
    isb

    ldr x0, =__kstack_top_hi
    mov sp, x0

    // 6. Absolute Jump to High Memory
    ldr x0, =el1_entry
    br x0
