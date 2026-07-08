#ifndef __KMALLOC_H__
#define __KMALLOC_H__

#include "macros.h"
#include "memory/paging.h"
#include "memory/virtual_memory.h"
#include "types/kmalloc_types.h"

// 32, 64, 128, 256, 512, 1024, 2048 slabs
#define CACHES      7

extern cache kcaches[CACHES];
extern uintptr_t kheap_start;

void kheap_init();
void* kmalloc(size_t bytes);
void kfree(void* ptr);

#endif
