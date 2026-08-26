#ifndef __KMALLOC_TYPES_H__
#define __KMALLOC_TYPES_H__

#include "macros.h"
#include "types/datastructures_types.h"

typedef struct{
    list_head_t list;
    u8 bitmap[16];
    u16 inuse;
    u16 total;
    u16 slab_order;                 // used to trace a pointer back to cache idx
} slab;

typedef struct{
    list_head_t full_slabs;
    list_head_t partial_slabs;
} cache;

#endif
