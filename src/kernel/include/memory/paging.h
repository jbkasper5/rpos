#ifndef __PAGING_H__
#define __PAGING_H__

#include "utils/datastructures.h"
#include "macros.h"
#include "io/kprintf.h"

typedef enum {
    PAGE_FREE = 0,
    PAGE_BUDDY = 1,
    PAGE_SLAB = 2,
    PAGE_RESERVED = 3,
} page_state;

typedef enum {
    PAGE_BUDDY_TAIL = (1 << 0),
    PAGE_BUDDY_HEAD = (1 << 1),
} page_flags;

typedef union {
    uint8_t value;
    struct {
        uint8_t state : 3;
        uint8_t flags : 5;
    } __attribute__((packed)) bits;
} page_frame_flags_t;



// order of 0 = 2^0 pages = 1 page
// order of 1 = 2^1 pages = 2 consecutive pages
// order of 2 = 2^2 pages = 4 consecutive pages
// ...
// order of 20 = 2^20 pages = 1048576 consecutive pages (all of RAM)

// struct is 24 bytes per page
// page is 4KiB = 2^12 bytes
// RAM is 4 GiB = 2^32 bytes
// page frame array must be 2^32 * 24 / 2^12 = 2^23 * 3 bytes
// if only using 1 GiB, it becomes 2^21 * 3 bytes
typedef struct {
    // 16 bytes
    list_head_t list;

    // 8 byte
    size_t order;

    // 4 bytes
    uint32_t refcount;

    // 1 byte
    page_frame_flags_t flags;

} page_frame_t; 



uintptr_t buddy_alloc(uint64_t bytes);
uintptr_t buddy_alloc_pt();
uint64_t initialize_page_frame_array();
uint8_t get_block_order(uint64_t addr);
void* head_from_page(void* page_addr);
page_state get_page_owner(void* page_addr);
void set_page_owner(void* page_addr, page_state new_owner);

#endif