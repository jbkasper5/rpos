#ifndef __PAGING_H__
#define __PAGING_H__

#include "utils/datastructures.h"
#include "macros.h"
#include "io/kprintf.h"
#include "memory/mmu.h"
#include "types/paging_types.h"


u64 buddy_alloc(u64 bytes);
void buddy_free(void* page);

u64 buddy_alloc_pt();
u64 initialize_page_frame_array();
u8 get_block_order(u64 addr);
void* head_from_page(void* page_addr);
page_state get_page_owner(void* page_addr);
void set_page_owner(void* page_addr, page_state new_owner);

void* clone_virtual_memory(pte* parent_table);
void reap_virtual_memory(pte* parent_table, u32 level);

#endif
