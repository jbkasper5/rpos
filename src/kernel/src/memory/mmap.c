#include "memory/mmap.h"

/**
 * @brief Rolls back a partial memory mapping made by the mapper functions on error.
 */
static void invalidate(){

}


/**
 * @brief Parses a flag bitfield into an appropriate table descriptor PTE
 * @param flags     A bitfield of flags
 * @return          A table descriptor bitfield with appropriate attributes set, only needs address
 */
static table_descriptor_t parse_table_flags(uint64_t flags){
    table_descriptor_t td = {0};

    if(!((flags & MAP_KERNEL) ^ (flags & MAP_USER))){
        ERROR("Incorrect flags supplied to map function: received 0x%x, expected exactly one of 0x%x or 0x%x\n", flags, MAP_KERNEL, MAP_USER);
        return td;
    }

    td.bits.valid = 1;
    td.bits.type = 1;
    td.bits.uxn = 1;
    td.bits.pxn = 1;
    td.bits.address = 0;

    if(flags & MAP_EXEC){
        if(flags & MAP_KERNEL){
            td.bits.pxn = 0;
        }else if(flags & MAP_USER){
            td.bits.uxn = 0;
        }
    }

    return td;
}

/**
 * @brief Parses a flag bitfield into an appropriate block/page descriptor PTE
 * @param flags     A bitfield of flags
 * @return          A memory descriptor (block/page) bitfield with appropriate attributes set, only needs address
 */
static mem_descriptor_t parse_block_flags(uint64_t flags, bool is_page){
    mem_descriptor_t md = {0};

    if(!((flags & MAP_KERNEL) ^ (flags & MAP_USER))){
        ERROR("Incorrect flags supplied to map function: received 0x%x, expected exactly one of 0x%x or 0x%x\n", flags, MAP_KERNEL, MAP_USER);
        return md;
    }

    md.bits.valid = 1;
    md.bits.af = 1;
    md.bits.ns = 0;
    md.bits.sh = 3;
    md.bits.ap = 0;
    md.bits.pxn = 1;
    md.bits.uxn = 1;
    md.bits.attr_index = 0;
    md.bits.type = is_page;

    if (flags & MAP_DEVICE){
        md.bits.attr_index = 1;
        md.bits.sh = 0;
    }

    if(flags & MAP_EXEC){
        if(flags & MAP_KERNEL){
            md.bits.pxn = 0;
        }else if(flags & MAP_USER){
            md.bits.uxn = 0;
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
bool map_pages(uint64_t virt_block, uint64_t phys_block, uint32_t blocks, uint64_t flags, uint64_t pt_base){
    // get pointer to page table, interpreted as table descriptor
    table_descriptor_t* l0_table = (table_descriptor_t*) pt_base;
    table_descriptor_t* l1_table;
    table_descriptor_t* l2_table;
    mem_descriptor_t* l3_table;

    table_descriptor_t ptte = parse_table_flags(flags);
    mem_descriptor_t ptme = parse_block_flags(flags, TRUE);

    uintptr_t va = (uintptr_t) virt_block;
    uintptr_t pa = (uintptr_t) phys_block;

    int idx0, idx1, idx2, idx3;

    for(int i = 0; i < blocks; i++){

        // pull indices from the page tables
        idx0 = (va >> 39) & 0x1FF;
        idx1 = (va >> 30) & 0x1FF;
        idx2 = (va >> 21) & 0x1FF;
        idx3 = (va >> 12) & 0x1FF;

        if(!l0_table[idx0].bits.valid){
            ptte.bits.address = ((uintptr_t) buddy_alloc_pt()) >> PAGE_SHIFT;
            l0_table[idx0] = ptte;
        }
        l1_table = (table_descriptor_t*) (l0_table[idx0].bits.address << PAGE_SHIFT);
        if (!l1_table[idx1].bits.valid) {
            ptte.bits.address = ((uintptr_t) buddy_alloc_pt()) >> PAGE_SHIFT;
            l1_table[idx1] = ptte;
        }

        l2_table = (table_descriptor_t*) (l1_table[idx1].bits.address << PAGE_SHIFT);
        if (!l2_table[idx2].bits.valid) {
            ptte.bits.address = ((uintptr_t) buddy_alloc_pt()) >> PAGE_SHIFT;
            l2_table[idx2] = ptte;
        }

        l3_table = (mem_descriptor_t*) (l2_table[idx2].bits.address << PAGE_SHIFT);
        if (!l3_table[idx3].bits.valid) {
            ptme.bits.address = (pa >> 12);
            l3_table[idx3] = ptme;
        }

        va += PAGE_SIZE;
        pa += PAGE_SIZE;
    }
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
bool map_blocks(uint64_t virt_block, uint64_t phys_block, uint32_t blocks, uint64_t flags, uint64_t pt_base){
    // get pointer to page table, interpreted as table descriptor
    table_descriptor_t* l0_table = (table_descriptor_t*) pt_base;
    table_descriptor_t* l1_table;
    mem_descriptor_t* l2_table;

    table_descriptor_t ptte = parse_table_flags(flags);
    mem_descriptor_t ptme = parse_block_flags(flags, FALSE);

    uintptr_t va = (uintptr_t) virt_block;
    uintptr_t pa = (uintptr_t) phys_block;

    int idx0, idx1, idx2;

    for(int i = 0; i < blocks; i++){

        // pull indices from the page tables
        idx0 = (va >> 39) & 0x1FF;
        idx1 = (va >> 30) & 0x1FF;
        idx2 = (va >> 21) & 0x1FF;

        if(!l0_table[idx0].bits.valid){
            ptte.bits.address = ((uintptr_t) buddy_alloc_pt()) >> PAGE_SHIFT;
            l0_table[idx0] = ptte;
        }
        l1_table = (table_descriptor_t*) (l0_table[idx0].bits.address << PAGE_SHIFT);
        if (!l1_table[idx1].bits.valid) {
            ptte.bits.address = ((uintptr_t) buddy_alloc_pt()) >> PAGE_SHIFT;
            l1_table[idx1] = ptte;
        }

        l2_table = (mem_descriptor_t*) (l1_table[idx1].bits.address << PAGE_SHIFT);
        if (!l2_table[idx2].bits.valid) {
            ptme.bits.address = (pa >> 12);
            l2_table[idx2] = ptme;
        }

        va += BLOCK_SIZE;
        pa += BLOCK_SIZE;
    }
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
bool map(uint64_t virt_block, uint64_t phys_block, uint8_t block_order, uint64_t flags, uint64_t pt_base){
    // block order of 9 means it's a 2MiB block and can be block allocated in an L2 table instead
    int idx3 = (virt_block >> 12) & 0x1FF;
    int iters = 0;
    bool (*mapping_func)(uint64_t, uint64_t, uint8_t, uint64_t, uint64_t);

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