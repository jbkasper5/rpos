#include "memory/kmalloc.h"
#include "memory/mmap.h"

uintptr_t kheap_start = 0xFFFF0000;
u64 kheap_size = 0;

cache kcaches[CACHES];

#define MIN_SLAB_ORDER      5
#define MAX_SLAB_ORDER      11
#define MAX_BUDDY_ORDER     18

static void* _slab_alloc(u32 order){
    // get new physical page from buddy allocator
    u64 phys_page = buddy_alloc(PAGE_SIZE * (order + 1));

    // the ownership changed hands to the slab
    set_page_owner((void*) phys_page, PAGE_SLAB);

    // if no more pages are available in RAM, PANIC
    if(!phys_page) panic();

    // map the page into memory
    map(phys_page, va_to_pa(phys_page), order, MAP_KERNEL | MAP_READ | MAP_WRITE, (u64) L0_TABLE);

    // for now, convert this to virtual later
    return (void*) phys_page;
}

void kheap_init(){
    // zero out all the slab pointers to start
    memset(kcaches, 0, CACHES * sizeof(cache));

    for(int i = 0; i < CACHES; i++){
        INIT_LIST_HEAD(&kcaches[i].full_slabs);
        INIT_LIST_HEAD(&kcaches[i].partial_slabs);
    }
}

static void* _addr_from_slab(slab* s){
    // once we have a slab, we need to read the bitfield to get the item offset from the slab
    // we also need to check if 

    for(int i = 0; i < s->total; i++){
        u8 byte = i / 8;
        u8 offset = i % 8;
        if(!(s->bitmap[byte] & (1 << offset))){
            s->bitmap[byte] |= (1 << offset);
            s->inuse++;

            // skip past metadata for the slab, then get the offset based on the bitfield
            return (void*) ((uintptr_t) s) + ALIGN_UP(sizeof(slab), (1 << s->slab_order)) + (i * (1 << s->slab_order));
        }
    }

    return NULL;
}

void* kmalloc(size_t bytes){
    if(bytes > (1ULL << (MAX_BUDDY_ORDER + PAGE_SHIFT))) return NULL;

    // align the requested number of bytes to the nearest slab_order    
    size_t aligned_bytes = 1ULL << (64 - __builtin_clzll(MAX(bytes, (size_t)1 << MIN_SLAB_ORDER) - 1));

    u32 log2 = log2_pow2(aligned_bytes);
    u32 cache_idx = log2 - MIN_SLAB_ORDER;

    // if aligned_bytes >= 4096, then log2 >= 12
    if(log2 > MAX_SLAB_ORDER){
        u64 pages = buddy_alloc(aligned_bytes);

        // need to map the page(s) into memory first
        map(pages, va_to_pa(pages), log2 - PAGE_SHIFT, MAP_KERNEL, (u64) L0_TABLE);

        return (void*) pages;
    }


    // no slabs yet
    if(list_empty(&kcaches[cache_idx].partial_slabs)){
        // allocate new slab (should be 1 page)
        u64 order = (log2 > 9) ? log2 - 9 : 0;
        slab* new_slab = (slab*) _slab_alloc(order);

        // zero the slab bitmap
        for(int i = 0; i < 8; i++) new_slab->bitmap[i] = 0;
        new_slab->inuse = 0;
        new_slab->total = ((PAGE_SIZE << order) - ALIGN_UP(sizeof(slab), aligned_bytes)) / aligned_bytes;
        new_slab->slab_order = log2;

        // add it to kcache partial slab list
        list_add(&new_slab->list, &kcaches[cache_idx].partial_slabs);
    }

    // once we have the partial slab, we need to "traverse" the bitfield
    // to find the first open allocation
    slab* s = list_entry(kcaches[cache_idx].partial_slabs.next, slab, list);
    uintptr_t addr = (uintptr_t) _addr_from_slab(s);

    // if this allocation filled the slab, we need to move it from partial to full
    if(s->inuse == s->total){
        list_remove(&s->list);
        list_add(&s->list, &kcaches[cache_idx].full_slabs);
    }

    return (void*) addr;
}

static void _slab_free(void* ptr){
    // right now we have a pointer pointing to somewhere in the middle of a slab
    // we need to find the head of the slab

    // first, we align ptr to a page boundary 
    // then we ask the buddy to give us the head of that page block since all slabs come from the buddy
    slab* slab_head = (slab*) (head_from_page((void*) (ALIGN_DOWN((u64) ptr, PAGE_SIZE))));

    // get the item size
    size_t item_size = 1 << slab_head->slab_order;

    // we now need to find how many items the metadata occupies
    size_t metadata_size = ALIGN_UP(sizeof(slab), item_size) / item_size;

    size_t offset = (((uintptr_t) ptr - (uintptr_t) slab_head) / item_size) - metadata_size;

    // mark the bit as free (1)
    size_t byte = offset / 8;
    size_t bit = offset % 8;

    // bit is not set, so it's already free
    if((slab_head->bitmap[byte] & (1 << bit)) == 0){
        return;
    }

    slab_head->bitmap[byte] &= ~(1 << bit);

    // if we're freeing from a full slab, we need to swap it from full list to partial list
    if(slab_head->inuse == slab_head->total){
        list_remove(&slab_head->list);
        list_add(&slab_head->list, &kcaches[slab_head->slab_order - MIN_SLAB_ORDER].partial_slabs);
    }

    // decrement the number of items in use
    slab_head->inuse--;

    // free the entire slab back to the buddy if nothing is in use
    if(slab_head->inuse == 0){
        list_remove(&slab_head->list);
        buddy_free((void*) slab_head);
    }
}

void kfree(void* ptr){
    // check dem nulls
    if(!ptr) return;

    void* aligned_ptr = (void*) ALIGN_DOWN((u64) ptr, PAGE_SIZE);
    page_state owner = get_page_owner(aligned_ptr);
    if(owner == PAGE_BUDDY){

        // TODO: need to munmap the virtual memory mappings for this page
        // munmap();

        buddy_free((void*) aligned_ptr);
        return;
    }else if(owner == PAGE_SLAB){
        _slab_free(ptr);
        return;
    }else if(owner == PAGE_FREE){
        WARNING("Attempting to free a free page\n");
        return;
    }

    panic();
}
