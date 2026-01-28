#include "memory/paging.h"
#include "memory/virtual_memory.h"
#include "memory/mem.h"

void* static_page_region_start();
void* static_page_region_end();
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
    DEBUG("PFN: %d\n", pfn);
    DEBUG("Buddy PFN: %d\n", buddy_pfn);


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

    // set the reference count to 1, since someone is requesting this page from the allocator
    block->refcount = 1;

    // declare ownership of the block to the buddy allocator
    block->flags.bits.state = PAGE_BUDDY;

    // convert the relative coordinate of the block within the frame metadata to a physical page address
    uint64_t pfn = block - frame_metadata;


    // mark the following pages as tail pages and define their offset from the head pfn
    for(int i = 1; i < (1U << req_order); i++){
        frame_metadata[pfn + i].flags.bits.flags = PAGE_BUDDY_TAIL;
        frame_metadata[pfn + i].refcount = 1;
        frame_metadata[pfn + i].order = i;
    }

    // return the pointer to the start of the allocated page
    return pfn << 12;
}


static void coalesce_up(page_frame_t* frame){
    size_t block_order = frame->order;

    // slide back to the previous frame in memory
    page_frame_t* prev_frame = frame - (1 << block_order);
    page_frame_t* next_frame = frame + (1 << block_order);

    if(prev_frame->order == block_order && prev_frame->flags.bits.state == PAGE_FREE && prev_frame->flags.bits.flags & PAGE_BUDDY_HEAD){
        list_remove(&frame->list);
        list_remove(&prev_frame->list);
        prev_frame->order++;

        // since we're coalescing, the later page frame becomes a tail page
        frame->flags.bits.state = PAGE_BUDDY_TAIL;

        // add congealed block to the next buddy list
        list_add(&prev_frame->list, &buddy_lists[block_order + 1]);

        // recurse up in case prev_frame also needs coalescing
        coalesce_up(prev_frame);
    }else if(FALSE){
        // check the frame ahead in case the one behind isn't suitable for coalescing
    }
}

/**
 * @brief Frees a block allocated by the buddy allocator
 * @param frame     Address of the block to free
 */
void buddy_free(page_frame_t* frame){
    // mark the frame as free
    // look in the buddy list for any free buddies of the same size
    // if the list contains a free buddy of the same size, check for coalescing
    // if coalescing is possible, remove existing buddy from the list, merge blocks, and promote order
    // then add the new block to the higher order buddy list

    // step 1: mark the frame as free
    frame->flags.bits.state = PAGE_FREE;

    // step 2: add it to the buddy list of the appropriate order
    size_t block_order = frame->order;

    // quick panic if block order exceeds the buddy list
    if(block_order > MAX_ORDER) panic();

    // add the frame to the free list
    list_add(&frame->list, &buddy_lists[block_order]);

    // perform any coalescing if necessary
    coalesce_up(frame);
}

/**
 * @brief Buddy allocator for allocating physical pages in powers of 2
 * @param bytes     Number of bytes to allocate 
 * @return          Address of the header page   
 */
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
    return NULL;
}


/**
 * @brief Allocates and zeroes a single page for a page table   
 * @return          Address of the page      
 */
uintptr_t buddy_alloc_pt(){
    uintptr_t pt = buddy_alloc(PAGE_SIZE);
    memset((void*) pt, 0, PAGE_SIZE);
    return pt;
}

void set_page_owner(void* page_addr, page_state new_owner){
    uint64_t pfn = (uint64_t) page_addr >> 12;

    if(frame_metadata[pfn].flags.bits.flags & PAGE_BUDDY_TAIL){
        // if it's a tail page, get the head page first
        pfn -= frame_metadata[pfn].order;
    }

    page_frame_t* pf = &frame_metadata[pfn];
    pf->flags.bits.state = new_owner;
}

page_state get_page_owner(void* page_addr){
    uint64_t pfn = (uint64_t) page_addr >> 12;

    if(frame_metadata[pfn].flags.bits.flags & PAGE_BUDDY_TAIL){
        // if it's a tail page, get the head page first
        pfn -= frame_metadata[pfn].order;
    }

    page_frame_t* pf = &frame_metadata[pfn];
    return pf->flags.bits.state;
}

void* head_from_page(void* page_addr){
    uint64_t pfn = (uint64_t) page_addr >> 12;

    page_frame_t* pf = &frame_metadata[pfn];

    // if we don't belong to the buddy or the slab, it wasn't allocated so we can't compute the head
    if (pf->flags.bits.state != PAGE_BUDDY && pf->flags.bits.state != PAGE_SLAB) {
        return NULL;
    }

    // if the page is already a head, return it
    if(pf->flags.bits.flags & PAGE_BUDDY_HEAD){
        return page_addr;
    }

    // otherwise, use the order to compute the head address
    size_t offset_from_head = pf->order;
    return (void*) ((pfn - offset_from_head) << 12);
}

static void _initialize_buddy_allocator(uint64_t start_page_addr, uint64_t available_pages){
    DEBUG("Initializing buddy allocator for 0x%x available pages...\n", available_pages)
    uint64_t curr_pfn = start_page_addr >> 12;
    page_frame_flags_t base_flags = {
        .bits.state = PAGE_FREE,
        .bits.flags = PAGE_BUDDY_HEAD
    };

    for(int64_t i = MAX_ORDER; i >= 0; i--){
        if(available_pages & (1ULL << i)){
            frame_metadata[curr_pfn].order = i;
            frame_metadata[curr_pfn].flags = base_flags;
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
    uint64_t reserved_memory = (uint64_t) static_page_region_end();
    uint64_t reserved_pages = (reserved_memory + 0xFFF) >> 12;
    uint64_t start_page_addr = (reserved_memory + 0xFFF) & (~0xFFF);

    // stay in the lower 1 GiB for now
    uint64_t available_pages = (1ULL << 18) - reserved_pages;

    // zero out the page frame metadata (which internally sets the state to free and )
    // for 1 GiB of RAM, there are 2^18 pages
    memset(frame_metadata, 0, (1 << 18) * sizeof(page_frame_t));

    _initialize_buddy_allocator(start_page_addr, available_pages);   
    return reserved_pages;
}
