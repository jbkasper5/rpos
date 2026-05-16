.section .text
.globl delay
delay:
    subs x0, x0, #1
    bne delay
    ret

.globl put32
put32:
    str w1, [x0]
    ret

.globl get32
get32:
    ldr w0, [x0]
    ret 

// put64(address, value)
.globl put64
put64:
    str x1, [x0]
    ret

.globl get64
get64:
    ldr x0, [x0]
    ret

.globl get_el
get_el:
    mrs x0, CurrentEL
    lsr x0, x0, #2
    ret

.globl mmu_enabled
mmu_enabled:
    mrs x0, SCTLR_EL1
    and x0, x0, #1      // isolate bit 0 (M bit)
    ret

.globl page_frame_array_start
page_frame_array_start:
    ldr x0, =__page_frame_array_start
    ret

.globl page_frame_array_end
page_frame_array_end:
    ldr x0, =__page_frame_array_end
    ret

.globl kernel_start
kernel_start:
    ldr x0, =__kernel_start
    ret

.globl get_bss_begin
get_bss_begin:
   ldr x0, =bss_begin
   ret

.globl get_bss_end
get_bss_end:
   ldr x0, =bss_end
   ret

.globl virt_base
virt_base:
    ldr x0, =VIRT_BASE
    ret


.globl va_to_pa
va_to_pa:
    stp x1, xzr, [sp, #-16]!
    ldr x1, =VIRT_BASE
    sub x0, x0, x1
    ldp x1, xzr, [sp], #16
    ret


.globl pa_to_va
pa_to_va:
    stp x1, xzr, [sp, #-16]!
    ldr x1, =VIRT_BASE
    add x0, x0, x1
    ldp x1, xzr, [sp], #16
    ret

.globl get_device_tree_start
get_device_tree_start:
    ldr x0, =DEVICE_TREE_BASE
    ret

.globl get_virt_test_region
get_virt_test_region:
    ldr x0, =TEST_BASE
    ret

.globl get_phys_test_region
get_phys_test_region:
    ldr x0, =__test_phys_loc
    ret

.globl get_test_size
get_test_size:
    ldr x0, =__test_size
    ret


.globl get_active_pcb
get_active_pcb:
    mrs x0, TPIDR_EL1
    ret


.globl set_active_pcb
set_active_pcb:
    msr TPIDR_EL1, x0
    ret


.globl flush_tlb
flush_tlb:
    // 1. Invalidate the TLB
    // 'vaae1is' means: Virtual Address, All ASIDs, EL1&0, Inner Shareable (all cores)
    // If you want to flush the whole thing:
    tlbi vmalle1is 

    // 2. Data Synchronization Barrier
    // Ensures the TLB invalidation completes before any further instructions run
    dsb sy

    // 3. Instruction Synchronization Barrier
    // Flushes the pipeline so the CPU fetches instructions using the new mapping rules
    isb

    
    ret


.globl ticks_since_boot
ticks_since_boot:
    mrs x0, cntvct_el0
    ret

.globl seconds_since_boot
seconds_since_boot:
    mrs x0, cntvct_el0
    mrs x1, cntfrq_el0

    scvtf d0, x0            // Signed Convert To Float: Move x0 -> d0 and convert to decimal
    scvtf d1, x1            // Signed Convert To Float: Move x1 -> d1 and convert to decimal
    
    fdiv  d0, d0, d1        // Floating-point divide: d0 = d0 / d1
    ret

.globl panic
panic:
    brk #0xBEEF