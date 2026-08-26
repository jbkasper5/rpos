.section ".text.boot"

.equ GICD_BASE,      0xff841000    // GIC distributor base address (for Pi 4)
.equ GICC_BASE,      0xff842000    // GIC CPU interface base address
.equ GICD_CTRLR,     0x000
.equ GICD_IGROUPR,   0x080
.equ GICC_CTRLR,     0x000
.equ GICD_ISENABLER, 0x100
.equ GICC_PMR,       0x004
.equ GICD_ITARGETSR, 0x800
.equ IT_NR,          128           // Number of interrupts supported (adjust if needed)

// ===========================
// GIC Setup Function
// ===========================
.globl setup_gic
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


    add x3, x2, #GICD_ITARGETSR
    mov w1, #0x01                 // Target Core 0
    strb w1, [x3, #96]            // Use strb to write just the byte for ID 96

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

    mov w1, #(1 << (27 % 32))
    str w1, [x2, #GICD_ISENABLER + (27 / 32) * 4]  // Enable interrupt 27

    ret
