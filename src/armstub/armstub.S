.globl _start
_start:
    bl setup_gic           // Call GIC setup before jumping to kernel
    ldr w0, kernel_entry32
    br x0


// ===========================
// Constants
// ===========================

.equ GICD_BASE,      0xff841000    // GIC distributor base address (for Pi 4)
.equ GICC_BASE,      0xff842000    // GIC CPU interface base address
.equ GICD_CTRLR,     0x000
.equ GICD_IGROUPR,   0x080
.equ GICC_CTRLR,     0x000
.equ GICD_ISENABLER, 0x100
.equ GICC_PMR,       0x004
.equ IT_NR,          128           // Number of interrupts supported (adjust if needed)

// ===========================
// GIC Setup Function
// ===========================
setup_gic:
    mrs x0, MPIDR_EL1
    ldr x2, =GICD_BASE

    mov w0, #3              // Enable group 0 and 1 IRQs
    str w0, [x2, #GICD_CTRLR]
2:
    ldr x1, =GICC_BASE
    mov w0, #0x3
    str w0, [x1, #GICC_CTRLR]   // Enable group 1 IRQs from CPU interface
    mov w0, #0xff
    str w0, [x1, #GICC_PMR]     // Allow all priorities

    // Set all interrupts to Group 1
    add x3, x2, #GICD_IGROUPR
    mov x0, #(IT_NR / 8)        // IT_NR = number of interrupts, /8 = number of IGROUPR registers
    mov w1, #~0
loop_group:
    subs x0, x0, #1
    str w1, [x3, x0, LSL #2]
    b.ne loop_group

    // ✅ Enable interrupt ID 30 (e.g., CNTPNSIRQ or a test IRQ)
    mov w1, #(1 << (30 % 32))
    str w1, [x2, #GICD_ISENABLER + (30 / 32) * 4]  // Enable interrupt 30

    ret




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
