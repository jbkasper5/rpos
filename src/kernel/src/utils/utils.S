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

.globl kstack_top_hi
kstack_top_hi:
    ldr x0, =__kstack_top_hi
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

.globl panic
panic:
    brk #0xBEEF