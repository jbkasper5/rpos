#include "memory/kmalloc.h"
#include "memory/mmap.h"

uintptr_t kheap_start = 0xFFFF0000;
uint64_t kheap_size = 0;

void sbrk(){
    // get new physical page from buddy allocator
    uint64_t phys_page = buddy_alloc(PAGE_SIZE);

    // find the next continuous page in the virtual heap
    uint64_t virt_mapping = ALIGN_UP(kheap_start + kheap_size, PAGE_SIZE);

    // map new allocated page to virtual heap extension
    map_pages(virt_mapping, phys_page, 1, MAP_KERNEL, L0_TABLE);
}

void kheap_init(){
    uint64_t phys_page = buddy_alloc(PAGE_SIZE);
    INFO("Allocated kernel heap at physical page: 0x%x\n", phys_page);

    map_pages(kheap_start, phys_page, 1, MAP_KERNEL, L0_TABLE);
    INFO("Mapped physical page to virtual address: 0x%x\n", kheap_start);

    kheap_size += PAGE_SIZE;
}

void* kmalloc(size_t bytes){
    size_t aligned_bytes = ALIGN(bytes, 8);
    INFO("Allocating %d bytes...\n", aligned_bytes);
    return NULL;
}

void kfree(void* ptr){
    if(!ptr) return;
}