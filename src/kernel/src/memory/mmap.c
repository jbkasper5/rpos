#include "memory/mmap.h"

// early boot uses 15 pages
u64 allocated_pages = 15;

extern char __static_page_region_start[];
extern char __static_page_region_end[];
#define STATIC_PAGE_REGION_PAGES 100   /* == __STATIC_PAGES */


extern u32 static_page_region_pages();
extern uintptr_t static_page_region_start();
extern u64 virt_base();

extern u64 pa_to_va(u64 pa);
extern u64 pa_to_va(u64 va);

/**
 * @brief Rolls back a partial memory mapping made by the mapper functions on error.
 */
// static void rollback(u64 virt_start, u64 blocks, u64 pt_base){

// }

static inline void clean_cached_pte(void *addr) {
    asm volatile("dsb ish" ::: "memory");
    asm volatile("dc cvac, %0" :: "r"(addr) : "memory");   // clean to PoC
}

static inline void clear_pte_tlb(u64 va){
    asm volatile("dsb ish" ::: "memory");                          // prior PTE write/clean done before invalidate
    asm volatile("tlbi vaae1is, %0" :: "r"(va >> 12) : "memory");  // evict the stale entry for this VA
    asm volatile("dsb ish" ::: "memory");                          // invalidate (and its broadcast) complete
    asm volatile("isb" ::: "memory");                              // next access re-walks -> new perms
}



uintptr_t alloc_page_table(){
    if (allocated_pages >= STATIC_PAGE_REGION_PAGES) panic();
    u64 page_addr = (uintptr_t)__static_page_region_start + (PAGE_SIZE * allocated_pages);
    allocated_pages++;
    memset((void*) page_addr, 0, PAGE_SIZE);

    // page tables are cacheable; clean the freshly-zeroed table to DRAM so the
    // non-cacheable walker sees zeros in the entries we don't explicitly write.
    asm volatile("dsb ish" ::: "memory");
    for (u64 off = 0; off < PAGE_SIZE; off += 64)   // 64 = cache line
        asm volatile("dc cvac, %0" :: "r"((u8*)page_addr + off) : "memory");
    asm volatile("dsb ish" ::: "memory");

    return va_to_pa(page_addr);
}


/**
 * @brief Parses a flag bitfield into an appropriate table descriptor PTE
 * @param flags     A bitfield of flags
 * @return          A table descriptor bitfield with appropriate attributes set, only needs address
 */
static pte parse_table_flags(u64 flags){
    pte td = {0};

    // if(!((flags & MAP_KERNEL) ^ (flags & MAP_USER))){
    //     ERROR("Incorrect flags supplied to map function: received 0x%x, expected exactly one of 0x%x or 0x%x\n", flags, MAP_KERNEL, MAP_USER);
    //     return td;
    // }

    td.td.valid = 1;
    td.td.type = 1;
    td.td.uxn = 0;
    td.td.pxn = 0;
    td.td.address = 0;

    if(flags & MAP_EXEC){
        if(flags & MAP_KERNEL){
            td.td.pxn = 0;
        }else if(flags & MAP_USER){
            td.td.uxn = 0;
        }
    }

    return td;
}

/**
 * @brief Parses a flag bitfield into an appropriate block/page descriptor PTE
 * @param flags     A bitfield of flags
 * @return          A memory descriptor (block/page) bitfield with appropriate attributes set, only needs address
 */
static pte parse_block_flags(u64 flags, bool is_page){
    pte md = {0};

    // if(!((flags & MAP_KERNEL) ^ (flags & MAP_USER))){
    //     ERROR("Incorrect flags supplied to map function: received 0x%x, expected exactly one of 0x%x or 0x%x\n", flags, MAP_KERNEL, MAP_USER);
    //     return md;
    // }

    // page is valid
    md.md.valid = 1;
    md.md.af = 1;
    md.md.ns = 0;
    md.md.sh = 3;
    md.md.attr_index = 0;
    md.md.type = is_page;

    // by default, kernel read only
    md.md.ap = EL0_NA_EL1_RO;

    // by default, not executable
    md.md.pxn = 1;
    md.md.uxn = 1;

    if (flags & MAP_DEVICE){
        md.md.attr_index = 1;
        md.md.sh = 0;
    }else if(flags & MAP_CACHE){
        md.md.attr_index = 2;
    }

    if(flags & MAP_EXEC){
        if(flags & MAP_KERNEL){
            md.md.pxn = 0;
        }else if(flags & MAP_USER){
            md.md.uxn = 0;
        }
    }

    if(flags & MAP_USER){
        md.md.ng = 1;
        if(flags & MAP_READ && flags & MAP_WRITE){
            md.md.ap = EL0_RW_EL1_RW;
        }else if(flags & MAP_READ){
            md.md.ap = EL0_RO_EL1_RO;
        }
    }else if(flags & MAP_KERNEL){
        if(flags & MAP_READ && flags & MAP_WRITE){
            md.md.ap = EL0_NA_EL1_RW;
        }else if(flags & MAP_READ){
            md.md.ap = EL0_NA_EL1_RO;
        }
    }

    return md;
}


/**
 * @brief Maps a virtual block to a physical block. Physical block must be an address returned by the buddy allocator.
 * 
 * @param virt_block    pointer to the virtual block
 * @param phys_block    index in the parent table to insert new entry
 * @param blocks        number of pages (4 KiB) to allocate
 * @param flags         PTE flags to use for the allocation
 * @param pt_base       address of the L0 page table to add this allocation to
 * @return              whether or not the allocation was successful
 */
bool map_pages(u64 virt_block, u64 phys_block, u32 blocks, u64 flags, u64 pt_base){
    // get pointer to page table, interpreted as table descriptor
    pte* l0_table = (pte*) pt_base;
    pte* l1_table;
    pte* l2_table;
    pte* l3_table;

    pte ptte = parse_table_flags(flags);
    pte ptme = parse_block_flags(flags, TRUE);

    uintptr_t va = (uintptr_t) virt_block;
    uintptr_t pa = (uintptr_t) phys_block;

    int idx0, idx1, idx2, idx3;

    for(int i = 0; i < blocks; i++){

        // pull indices from the page tables
        idx0 = (va >> 39) & 0x1FF;
        idx1 = (va >> 30) & 0x1FF;
        idx2 = (va >> 21) & 0x1FF;
        idx3 = (va >> 12) & 0x1FF;

        if(!l0_table[idx0].td.valid){
            ptte.td.address = (va_to_pa(buddy_alloc_pt())) >> PAGE_SHIFT;
            l0_table[idx0] = ptte;
            clean_cached_pte(&l0_table[idx0]);
        }
        
        l1_table = (pte*) (((u64) l0_table[idx0].td.address << PAGE_SHIFT) + virt_base());
        if (!l1_table[idx1].td.valid) {
            ptte.td.address = (va_to_pa(buddy_alloc_pt())) >> PAGE_SHIFT;
            l1_table[idx1] = ptte;
            clean_cached_pte(&l1_table[idx1]);
        }

        l2_table = (pte*) (((u64) l1_table[idx1].td.address << PAGE_SHIFT) + virt_base());
        if (!l2_table[idx2].td.valid) {
            ptte.td.address = (va_to_pa(buddy_alloc_pt())) >> PAGE_SHIFT;
            l2_table[idx2] = ptte;
            clean_cached_pte(&l2_table[idx2]);
        }

        l3_table = (pte*) (((u64) l2_table[idx2].td.address << PAGE_SHIFT) + virt_base());
        if (!l3_table[idx3].md.valid) {
            ptme.md.address = (pa >> PAGE_SHIFT);
            l3_table[idx3] = ptme;
            clean_cached_pte(&l3_table[idx3]);
        }else{
            // WARNING("ENTRY ALREADY EXISTS AT THIS L3 INDEX: %d\n", idx3);
            // ptme.md.address = l3_table[idx3].md.address;
            // if(ptme.value != l3_table[idx3].md.value){

            //     // update permissions
            //     l3_table[idx3] = ptme;
            //     clean_cached_pte(&l3_table[idx3]);
        
            // }
        }

        va += PAGE_SIZE;
        pa += PAGE_SIZE;
    }
    asm volatile("dsb ish" ::: "memory");
    return TRUE;
}


/**
 * @brief Maps a virtual block to a physical block. Physical block must be an address returned by the buddy allocator.
 * 
 * @param virt_block    pointer to the virtual block
 * @param phys_block    index in the parent table to insert new entry
 * @param blocks        number of L2 blocks (1 MiB) to allocate
 * @param flags         PTE flags to use for the allocation
 * @param pt_base       address of the L0 page table to add this allocation to
 * @return              whether or not the allocation was successful
 */
bool map_blocks(u64 virt_block, u64 phys_block, u32 blocks, u64 flags, u64 pt_base){
    // get pointer to page table, interpreted as table descriptor
    pte* l0_table = (pte*) pt_base;
    pte* l1_table;
    pte* l2_table;

    pte ptte = parse_table_flags(flags);
    pte ptme = parse_block_flags(flags, FALSE);

    uintptr_t va = (uintptr_t) virt_block;
    uintptr_t pa = (uintptr_t) phys_block;

    int idx0, idx1, idx2;

    for(int i = 0; i < blocks; i++){

        // pull indices from the page tables
        idx0 = (va >> 39) & 0x1FF;
        idx1 = (va >> 30) & 0x1FF;
        idx2 = (va >> 21) & 0x1FF;

        if(!l0_table[idx0].td.valid){
            ptte.td.address = (va_to_pa(buddy_alloc_pt())) >> PAGE_SHIFT;
            l0_table[idx0] = ptte;
            clean_cached_pte(&l0_table[idx0]);
        }
        l1_table = (pte*) (((u64) l0_table[idx0].td.address << PAGE_SHIFT) + virt_base());
        if (!l1_table[idx1].td.valid) {
            ptte.td.address = (va_to_pa(buddy_alloc_pt())) >> PAGE_SHIFT;
            l1_table[idx1] = ptte;
            clean_cached_pte(&l1_table[idx1]);
        }

        l2_table = (pte*) (((u64) l1_table[idx1].td.address << PAGE_SHIFT) + virt_base());
        if (!l2_table[idx2].td.valid) {
            ptme.md.address = (pa >> PAGE_SHIFT);
            l2_table[idx2] = ptme;
            clean_cached_pte(&l2_table[idx2]);
        }

        va += BLOCK_SIZE;
        pa += BLOCK_SIZE;
    }
    asm volatile("dsb ish" ::: "memory");
    return TRUE;
} 


/**
 * @brief Maps a virtual block to a physical block. Physical block must be an address returned by the buddy allocator.
 * 
 * @param virt_block    pointer to the virtual block
 * @param phys_block    index in the parent table to insert new entry
 * @param block_order   order of the block. Describes number of consecutive pages as a power of 2 (i.e. order 2 = 4 pages, order 3 = 8 pages, etc.)
 * @param flags         PTE flags to use for the allocation
 * @param pt_base       address of the L0 page table to add this allocation to
 * @return              whether or not the allocation was successful
 */
bool map(u64 virt_block, u64 phys_block, u8 block_order, u64 flags, u64 pt_base){
    // block order of 9 means it's a 2MiB block and can be block allocated in an L2 table instead
    int idx3 = (virt_block >> 12) & 0x1FF;
    int iters = 0;
    bool (*mapping_func)(u64, u64, u32, u64, u64);


    // TODO: in case of any intermediate allocations or remappings, needs to figure out 
    // whether a block is optimal, even if the order is >= 9
    if(block_order >= 9 && idx3 == 0){
        mapping_func = map_blocks;
        // get number of blocks to map
        iters = 1 << (block_order - 9);
    }else{
        mapping_func = map_pages;
        // get number of pages to map
        iters = 1 << (block_order);
    }

    return mapping_func(virt_block, phys_block, iters, flags, pt_base);
}


/// @brief Update the permissions for a provided virtual memory range. Assumes all mappings are L3
/// @param virt_start   64 bit virtual address defining the start of the range. Should be page aligned.
/// @param virt_end     64 bit virtual address defining the end of the range. Should be page aligned.
/// @param flags        Bitfield of permissions. Uses same conventions as `map()`
/// @param pt_base      Base L0 page table to use
/// @return             True on success, False on failure
bool update_mapping(u64 virt_start, u64 virt_end, u64 flags, u64 pt_base){
    if((virt_start & 0xFFF) || (virt_end & 0xFFF)){
        WARNING("virt_start or virt_end not page-aligned: (0x%x-0x%x)\n", virt_start, virt_end);
        return FALSE;
    }

    pte* l0_table = (pte*) pt_base;
    pte* l1_table;
    pte* l2_table;
    pte* l3_table;

    pte ptme = parse_block_flags(flags, TRUE);

    uintptr_t va = (uintptr_t) virt_start;

    int idx0, idx1, idx2, idx3;

    u64 pages = (virt_end - virt_start) / PAGE_SIZE;

    for(int i = 0; i < pages; i++){

        // pull indices from the page tables
        idx0 = (va >> 39) & 0x1FF;
        idx1 = (va >> 30) & 0x1FF;
        idx2 = (va >> 21) & 0x1FF;
        idx3 = (va >> 12) & 0x1FF;


        // walk the page tables 
        if(!l0_table[idx0].td.valid) return FALSE;
        l1_table = pa_to_va(l0_table[idx0].td.address << PAGE_SHIFT);

        if(!l1_table[idx1].td.valid) return FALSE;
        l2_table = pa_to_va(l1_table[idx1].td.address << PAGE_SHIFT);

        if(!l2_table[idx2].td.valid) return FALSE;
        l3_table = pa_to_va(l2_table[idx2].td.address << PAGE_SHIFT);

        pte entry = l3_table[idx3];
        if(!entry.md.valid) {
            ERROR("Trying to update permissions for unmapped memory at va 0x%x\n", va);
            return FALSE;
        }
        
        ptme.md.address = entry.md.address;
        l3_table[idx3].value = ptme.value;
        clear_pte_tlb(va);
    }
}