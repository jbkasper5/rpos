#ifndef __LINKER_SYMBOLS_H__
#define __LINKER_SYMBOLS_H__

#include "macros.h"

#define VIRT_BASE 0xFFFF800000000000ULL

// Absolute (linked) address of a symbol
#define SYM_ABS(name)  ({ u64 _a; __asm__ volatile("ldr %0, =" #name : "=r"(_a)); _a; })

// physical address of a high-half symbol at boot
#define SYM_PHYS(name) (SYM_ABS(name) - VIRT_BASE)

#define SYM_VIRT(name) ((u64)(name) + VIRT_BASE)

// runtime (MMU on) address of a symbol
#define SYM_VA(name)   ((u64)(name))

// boot image (low physical)
extern char __boot_text_start[],    __boot_text_end[];      /* R-X */
extern char __boot_data_start[],    __boot_data_end[];      /* R-W */
extern char __boot_rodata_start[],  __boot_rodata_end[];    /* R-- */
extern char __boot_bss_start[],     __boot_bss_end[];       /* R-W (NOLOAD) */

// kernel image (high-half VMA, low LMA)
extern char __kernel_start[],       __kernel_end[];
extern char __kernel_high_start[],  __kernel_high_end[];

extern char __text_start[],         __text_end[];           /* R-X */
extern char __rodata_start[],       __rodata_end[];         /* R-- (incl. .font) */
extern char __font_start[],         __font_end[];
extern char __data_start[],         __data_end[];           /* R-W loadable group */
extern char __bss_start[],          __bss_end[];

// reserved regions (NOLOAD)
extern char __kstack_bottom[],      __kstack_top[];
extern char __kstack_bottom_hi[],   __kstack_top_hi[];
extern char __page_frame_array_start[],   __page_frame_array_end[];
extern char __static_page_region_start[], __static_page_region_end[];
extern char __static_page_region_start_virt[];
extern char __static_page_region_start_phys[];

// test section 
extern char __test_phys_loc[], __test_size[];

#endif
