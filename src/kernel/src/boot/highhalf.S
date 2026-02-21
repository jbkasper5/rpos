# # param x0: physical address of the kernel L0 page table

# .section ".text.boot"

# .globl enable_mmu
# enable_mmu:
#     stp x29, x30, [sp, #-16]!

#     msr TTBR1_EL1, x0

#     ldr x0, =0x80100010
#     msr TCR_EL1, x0

#     isb

#     ldr x0, =((0xFF << 0) | (0x04 << 8))
#     msr MAIR_EL1, x0

#     dsb ish
#     isb

#     mrs x0, sctlr_el1
#     orr x0, x0, #(1 << 0)      // M (MMU enable)
#     msr sctlr_el1, x0

#     isb

#     ldp x29, x30, [sp], #16

#     # ldr x0, =__kstack_high
#     # mov sp, x0    
#     eret