#include "memory/paging.h"

void* page_frame_array_start();
void* page_frame_array_end();
void* kernel_start();
void* kernel_end();

#define MAX_ORDER 20

// one list per order
static list_head_t buddy_lists[MAX_ORDER + 1];
static page_frame_t* frame_metadata;

// split a higher order block in 2
uintptr_t _split_down(uint8_t req_order, uint8_t curr_order){
    // get the page frame number in the larger array
    page_frame_t* og_frame = list_entry(buddy_lists[curr_order].next, page_frame_t, list);
    if(req_order == curr_order){
        // placeholder
        return (uintptr_t) &og_frame->list;
    }
    uint32_t pfn = og_frame - frame_metadata;
    uint32_t buddy_pfn = pfn + (1UL << (curr_order - 1));
    PDEBUG("PFN: %d\n", pfn);
    PDEBUG("Buddy PFN: %d\n", buddy_pfn);


    // TODO: at some point, verify if any coalescing is required as we split blocks downward
    list_remove(buddy_lists[curr_order].next);
    list_add(&frame_metadata[buddy_pfn].list, &buddy_lists[curr_order - 1]);
    list_add(&og_frame->list, &buddy_lists[curr_order - 1]);

    frame_metadata[pfn].order = curr_order - 1;
    frame_metadata[buddy_pfn].order = curr_order - 1;
    return _split_down(req_order, curr_order - 1);
}

uintptr_t _alloc_and_return(list_head_t* head, uint32_t req_order){
    // get the correctly sized frame from the buddy list
    page_frame_t* block = list_entry(buddy_lists[req_order].next, page_frame_t, list);

    // remove the frame from the buddy list since it's now allocated
    list_remove(&block->list);

    // mark the block as allocated
    block->flags.bits.allocated = TRUE;

    uint64_t pfn = block - frame_metadata;

    // return the pointer to the start of the allocated page
    return pfn << 12;
}

void buddy_free(page_frame_t* frame){
    // mark the frame as free
    // look in the buddy list for any free buddies of the same size
    // if the list contains a free buddy of the same size, check for coalescing
    // if coalescing is possible, remove existing buddy from the list, merge blocks, and promote order
    // then add the new block to the higher order buddy list
}

// page allocator - uses buddy allocation
uintptr_t buddy_alloc(uint64_t bytes){
    // round bytes up to the nearest page granule
    uint32_t n_pages = (bytes + 4095) >> 12;  // For 4 KiB pages

    // convert number of pages into minimum order
    uint8_t requested_order = 0;
    while ((1U << requested_order) < n_pages) requested_order++;

    if(!list_empty(&buddy_lists[requested_order])) return _alloc_and_return(&buddy_lists[requested_order], requested_order);

    for(int i = requested_order + 1; i <= MAX_ORDER; i++){
        // if the buddy list at order i is defined, split it down to match the requested order
        // otherwise, try the next highest order
        if(!list_empty(&buddy_lists[i])) return _alloc_and_return((list_head_t*) _split_down(requested_order, i), requested_order);
    }
    
    // at this point, no free blocks exist of the requested size
    return 0;
}

static void _initialize_buddy_allocator(uint64_t start_page_addr, uint64_t available_pages){
    PDEBUG("Initializing buddy allocator for 0x%x available pages...\n", available_pages)
    uint64_t curr_pfn = start_page_addr >> 12;
    for(int64_t i = MAX_ORDER; i >= 0; i--){
        if(available_pages & (1ULL << i)){
            frame_metadata[curr_pfn].order = i;
            list_add(&frame_metadata[curr_pfn].list, &buddy_lists[i]);
            curr_pfn += (1ULL << i);
        }
    }
}

uint8_t get_block_order(uint64_t addr){
    if(addr & 0xFFF){
        return -1;
    }

    uint64_t pfn = addr >> 12;
    return frame_metadata[pfn].order;
}

uint64_t initialize_page_frame_array(){
    for(int i = 0; i <= MAX_ORDER; i++) INIT_LIST_HEAD(&buddy_lists[i]);
    frame_metadata = (page_frame_t*) page_frame_array_start();
    uint64_t reserved_memory = (uint64_t) page_frame_array_end();
    uint64_t reserved_pages = (reserved_memory + 0xFFF) >> 12;
    uint64_t start_page_addr = (reserved_memory + 0xFFF) & (~0xFFF);

    // stay in the lower 1 GiB for now
    uint64_t available_pages = (1ULL << 18) - reserved_pages;
    _initialize_buddy_allocator(start_page_addr, available_pages);   
    return reserved_pages;
}