#ifndef __PAGING_H__
#define __PAGING_H__

#include "datastructures.h"
#include "macros.h"
#include "printf.h"

typedef enum page_mutability_s{
    IMMUTABLE = 0,
    MUTABLE = 1,
} page_mutability_t;

typedef union {
    uint8_t value;
    struct {
        uint8_t owner         : 1;
        uint8_t allocated     : 1;
        uint8_t mutability    : 1;
        uint8_t reserved      : 5;
    } __attribute__((packed)) bits;
} page_frame_flags_t;



// order of 0 = 2^0 pages = 1 page
// order of 1 = 2^1 pages = 2 consecutive pages
// order of 2 = 2^2 pages = 4 consecutive pages
// ...
// order of 20 = 2^20 pages = 1048576 consecutive pages (all of RAM)
typedef struct page_frame_s{
    // 1 byte
    page_frame_flags_t flags;

    // 1 byte
    uint8_t order;

    // 16 bytes
    list_head_t list;
} page_frame_t; 



uintptr_t buddy_alloc(uint64_t bytes);
void initialize_page_frame_array();

#endif