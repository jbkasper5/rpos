.section ".text.boot"

.globl delay_lo
delay_lo:
    subs x0, x0, #1
    bne delay_lo
    ret

.globl static_page_region_start
static_page_region_start:
    ldr x0, =__static_page_region_start
    ret

.globl static_page_region_end
static_page_region_end:
    ldr x0, =__static_page_region_end
    ret

.globl static_page_region_start_phys
static_page_region_start_phys:
    ldr x0, =__static_page_region_start_phys
    ret


.globl static_page_region_pages
static_page_region_pages:
    ldr x0, =__STATIC_PAGES
    ret

.globl virt_base
virt_base:
    ldr x0, =VIRT_BASE
    ret

.globl kernel_high_start
kernel_high_start:
    ldr x0, =__kernel_high_start
    ret

.globl kernel_high_end
kernel_high_end:
    ldr x0, =__kernel_high_end
    ret

.globl kernel_end
kernel_end:
    ldr x0, =__kernel_end
    ret
