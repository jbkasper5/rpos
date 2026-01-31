#ifndef __KMALLOC_H__
#define __KMALLOC_H__

#include "macros.h"
#include "memory/paging.h"
#include "memory/virtual_memory.h"

// 32, 64, 128, 256, 512, 1024, 2048 slabs
#define CACHES      7

typedef struct{
    list_head_t list;
    uint8_t bitmap[16];
    uint16_t inuse;
    uint16_t total;
    uint16_t slab_order;                 // used to trace a pointer back to cache idx
} slab;

typedef struct{
    list_head_t full_slabs;
    list_head_t partial_slabs;
} cache;

extern cache kcaches[CACHES];
extern uintptr_t kheap_start;

void kheap_init();
void* kmalloc(size_t bytes);
void kfree(void* ptr);

#endif