#include "paging.h"

void* page_frame_array_start();
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

    // return the frame
    return (uintptr_t) block;
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

void initialize_page_frame_array(){
    frame_metadata = page_frame_array_start();
    frame_metadata->order = MAX_ORDER;
    INIT_LIST_HEAD(&frame_metadata->list);
    for (int i = 0; i <= MAX_ORDER; i++) INIT_LIST_HEAD(&buddy_lists[i]);
    list_add(&frame_metadata->list, &buddy_lists[MAX_ORDER]);

    uint64_t kernel_size = (uint64_t) kernel_end();
    PDEBUG("Kernel size: 0x%x\n", kernel_size);
    PDEBUG("Frame metadata start: 0x%x\n", frame_metadata);
    page_frame_t* frame = (page_frame_t*) buddy_alloc(kernel_size);

    // calculate page index in the metadata
    uint64_t physical_page_start = (frame - frame_metadata);

    // shift left by 12 to get physical address of the page
    physical_page_start <<= 12;
    
    PDEBUG("Allocated kernel memory. Block start: 0x%x, block order: %d\n", physical_page_start, frame->order);
}