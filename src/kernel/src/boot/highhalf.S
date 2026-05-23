.section ".text.boot"

.globl enable_mmu
enable_mmu:
    // 1. Setup MAIR (Index 0 = Normal WB/WA/RA)
    ldr x2, =((0xFF << 0) | (0x04 << 8))
    msr MAIR_EL1, x2

    // 2. Setup TCR. Pre-OR'd into one 64-bit literal:
    //      T0SZ=16            (48-bit VA, low half)        0x10
    //      IRGN0=01,ORGN0=01  (cacheable walks)            0x100|0x400
    //      SH0=11             (inner shareable)            0x3000
    //      T1SZ=16            (48-bit VA, high half)       0x100000
    //      IRGN1=01,ORGN1=01,SH1=11                        0x1000000|0x4000000|0x30000000
    //      TG1=10             (4KB granule, TTBR1)         0x80000000
    //      IPS=010            (40-bit PA - covers PCIe @ 24GiB) 0x200000000
    //
    //    The previous value 0x80100010 only set T0SZ/T1SZ/TG1; IPS was 0,
    //    capping output PAs at 32 bits and breaking any mapping above 4
    //    GiB (such as the PCIe outbound window at 0x600000000).
    ldr x2, =0x2B5103510
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

    // 5. Enable MMU
    mrs x2, sctlr_el1
    orr x2, x2, #(1 << 0)       // M (MMU enable)
    msr sctlr_el1, x2
    isb

    ldr x0, =0xFFFF8000FF841000
    at s1e1r, x0
    isb
    mrs x1, par_el1 // 0x80F

    ldr x0, =0xFFFF8000FF842000
    at s1e1r, x0
    isb
    mrs x1, par_el1 // 0x80F

    ldr x0, =__kstack_top_hi
    mov sp, x0

    // 6. Absolute Jump to High Memory
    ldr x0, =el1_entry
    br x0
