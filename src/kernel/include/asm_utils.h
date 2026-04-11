#ifndef __ASM_UTILS_H__
#define __ASM_UTILS_H__

#include <stdint.h>

extern void delay(u64 ticks);

extern u32 get_el();

extern u64 page_frame_array_start();
extern u64 page_frame_array_end();

extern u64 static_page_region_start();
extern u64 static_page_region_end();
extern u32 static_page_region_pages();

extern u64 virt_base();

extern u64 kernel_start();
extern u64 kernel_end();

extern u64 get_bss_begin();
extern u64 get_bss_end();

extern u64 get_device_tree_start();

extern u64 get_virt_test_region();
extern u64 get_phys_test_region();
extern u64 get_test_size();

extern u64 get_core_pcb();

#endif