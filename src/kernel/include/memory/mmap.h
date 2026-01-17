#ifndef __MMAP_H__
#define __MMAP_H__

#include "macros.h"
#include "memory/mmu.h"
#include "memory/virtual_memory.h"
#include "memory/paging.h"

// virtual memory flags
#define MAP_READ        (1UL << 0)
#define MAP_WRITE       (1UL << 1)
#define MAP_EXEC        (1UL << 2)
#define MAP_USER        (1UL << 3)
#define MAP_KERNEL      (1UL << 4)
#define MAP_DEVICE      (1UL << 5)
#define MAP_NOCACHE     (1UL << 6)
#define MAP_WRITE_COMB  (1UL << 7)

// maps a virtual block to a physical block
// physical blocks are expected to be returned from the buddy allocator
bool map(uint64_t virt_block, uint64_t phys_block, uint8_t block_order, uint64_t flags, uint64_t pt_base);
bool map_pages(uint64_t virt_block, uint64_t phys_block, uint32_t blocks, uint64_t flags, uint64_t pt_base);
bool map_blocks(uint64_t virt_block, uint64_t phys_block, uint32_t blocks, uint64_t flags, uint64_t pt_base);
uintptr_t alloc_page_table();

#endif