#ifndef __KMALLOC_H__
#define __KMALLOC_H__

#include "macros.h"
#include "memory/paging.h"
#include "memory/virtual_memory.h"

extern uintptr_t kheap_start;

void kheap_init();
void* kmalloc(size_t bytes);
void kfree(void* ptr);

#endif