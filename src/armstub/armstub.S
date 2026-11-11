.globl _start
_start:
    // set up the cache for all cores
    mrs x0, S3_1_C11_C0_2    // L2CTLR_EL1
    mov x1, #0x22            // data RAM latency + setup, as in the stock armstub8
    orr x0, x0, x1
    msr S3_1_C11_C0_2, x0
    isb

    ldr w1, kernel_entry32
    br x1

// ===========================
// Required symbols
// ===========================

.ltorg

.org 0xf0
.globl stub_magic
stub_magic:
    .word 0x5afe570b

.org 0xf4
.globl stub_version
stub_version:
    .word 0

.org 0xfc
.globl kernel_entry32
kernel_entry32:
    .word 0x0
