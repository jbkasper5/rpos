#ifndef __ASM_UTILS_H__
#define __ASM_UTILS_H__

#include <stdint.h>

extern void delay(uint64_t ticks);

extern uint32_t get_el();

extern uint64_t page_frame_array_start();
extern uint64_t page_frame_array_end();

extern uint64_t static_page_region_start();
extern uint64_t static_page_region_end();
extern uint32_t static_page_region_pages();

extern uint64_t virt_base();

extern uint64_t kernel_start();
extern uint64_t kernel_end();

extern uint64_t get_bss_begin();
extern uint64_t get_bss_end();

extern uint64_t get_device_tree_start();

extern uint64_t get_virt_test_region();
extern uint64_t get_phys_test_region();
extern uint64_t get_test_size();

#endif