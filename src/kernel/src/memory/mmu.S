.section .data
.align 2
debug_str:
    .asciz "HERE!\n"

.section .text
/* NOTE: The real TCR_EL1 value is loaded as a literal in boot/highhalf.S
 * (0x2B5103510). This symbol used to live here for reference but was
 * confusing because it looked like the live config when nothing actually
 * referenced it. Removed.
 */

.globl walk
walk:
    // stage 1 el0 read translation
    at S1E0R, x0
    mrs x0, PAR_EL1
    ret


.globl assume_l0_table
# param $x0 - address of L0 table to assume
assume_l0_table:
    msr TTBR1_EL1, x0
    dsb ish
    isb
    ret