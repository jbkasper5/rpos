#include "memory/kmalloc.h"
#include "memory/mmap.h"

uintptr_t kheap_start = 0xFFFF0000;
uint64_t kheap_size = 0;

cache kcaches[CACHES];

#define MIN_SLAB_ORDER      5

static void* _slab_alloc(uint32_t order){
    // get new physical page from buddy allocator
    uint64_t phys_page = buddy_alloc(PAGE_SIZE * order);

    // if no more pages are available in RAM, PANIC
    if(!phys_page) panic();

    // map the page into memory
    map(phys_page, phys_page, 0, MAP_KERNEL, L0_TABLE);

    // for now, convert this to virtual later
    return phys_page;
}

void kheap_init(){
    // zero out all the slab pointers to start
    memset(kcaches, 0, CACHES, sizeof(cache));

    for(int i = 0; i <= CACHES; i++){
        INIT_LIST_HEAD(&kcaches[i].full_slabs);
        INIT_LIST_HEAD(&kcaches[i].partial_slabs);
    }
}

static void* _addr_from_slab(slab* s){
    // once we have a slab, we need to read the bitfield to get the item offset from the slab
    // we also need to check if 

    for(int i = 0; i < s->total; i++){
        uint8_t byte = i / 8;
        uint8_t offset = i % 8;
        if(!(s->bitmap[byte] & (1 << offset))){
            INFO("Found free block at index %d\n", i);
            s->bitmap[byte] |= (1 << offset);
            s->inuse++;

            // skip past metadata for the slab, then get the offset based on the bitfield
            return ((uintptr_t) s) + ALIGN_UP(sizeof(slab), (1 << s->slab_order)) + (i * (1 << s->slab_order));
        }
    }
}

void* kmalloc(size_t bytes){
    size_t aligned_bytes = 1;
    uint32_t shift = 1;

    // align the requested number of bytes to the nearest slab_order
    while (aligned_bytes < bytes) aligned_bytes <<= 1;
    aligned_bytes = MAX(aligned_bytes, (1 << MIN_SLAB_ORDER));
    DEBUG("Allocating %d bytes...\n", aligned_bytes);

    uint32_t log2 = log2_pow2(aligned_bytes);
    uint32_t cache_idx = log2 - MIN_SLAB_ORDER;


    // no slabs yet
    if(list_empty(&kcaches[cache_idx].partial_slabs)){
        // allocate new slab
        slab* new_slab = (slab*) _slab_alloc(PAGE_SIZE);

        // zero the slab bitmap
        for(int i = 0; i < 8; i++) new_slab->bitmap[i] = 0;
        new_slab->inuse = 0;
        new_slab->total = (PAGE_SIZE - ALIGN_UP(sizeof(slab), aligned_bytes)) / aligned_bytes;
        new_slab->slab_order = log2;

        // add it to kcache partial slab list
        list_add(&new_slab->list, &kcaches[cache_idx].partial_slabs);
    }else{
        slab* partial_slab = list_entry(kcaches[cache_idx].partial_slabs.next, slab, list);
        
    }

    // once we have the partial slab, we need to "traverse" the bitfield
    // to find the first open allocation
    slab* s = list_entry(kcaches[cache_idx].partial_slabs.next, slab, list);
    uintptr_t addr = (uintptr_t) _addr_from_slab(s);
    INFO("Allocated address 0x%x\n", addr);
    return addr;
}

void kfree(void* ptr){
    
}
