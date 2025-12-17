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
    adrp x0, __page_frame_array_start
    ret

.globl page_frame_array_end
page_frame_array_end:
    adrp x0, __page_frame_array_end
    ret

.globl kernel_start
kernel_start:
    adrp x0, __kernel_start
    ret

.globl kernel_end
kernel_end:
    adrp x0, __kernel_end
    ret