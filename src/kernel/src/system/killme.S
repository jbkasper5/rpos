#include "memory/mm.h"

.section .text
.globl drop_to_user

// params: 
    // x0 -> new kstack
drop_to_user:
    // surgical clear of TLB mappings for specific ASID
    # mov x5, #0
    # tlbi aside1is, x5
    # dsb ish
    # isb

    # // switch out new table
    msr TTBR0_EL1, x1
    dsb ish
    isb


    # // nuke to fully wipe the TLB
    tlbi vmalle1is
    dsb ish
    isb


    mov sp, x0
    b ret_from_fork