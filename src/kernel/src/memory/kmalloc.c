#include "memory/kmalloc.h"

void* kheap_start;

void kheap_init(){
    kheap_start = (void*) buddy_alloc(PAGE_SIZE);
}

void* kmalloc(size_t bytes){
    // size_t aligned_bytes = ALIGN(bytes, 8);
    return NULL;
}

void kfree(void* ptr){
    if(!ptr) return;
}