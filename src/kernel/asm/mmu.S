#include "sysregs.h"

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
    adrp x0, _page_table_base
    msr TTBR0_EL1, x0

    bl init_mmu

    # mrs x0, SCTLR_EL1
    # mov x1, #SCTLR_MMU_ENABLED
    # orr x0, x0, x1
    # msr SCTLR_EL1, x0

    ldp x29, x30, [sp], #16
    ret
