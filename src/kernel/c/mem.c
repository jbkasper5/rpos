#include "mem.h"
#include "mmu.h"
#include "mm.h"
#include "peripherals/base.h"

void* memcpy(void* dest, const void* src, uint32_t bytes) {
    char* cdest = (char*)dest;
    const char* csrc = (const char*)src;
    for (uint32_t i = 0; i < bytes; i++) {
        cdest[i] = csrc[i];
    }
    return dest;
}

void* memset(void* ptr, uint64_t bytes, int8_t value){
    unsigned char* addr = (unsigned char*) ptr;
    for(uint64_t i = 0; i < bytes; i++) addr[i] = value;
    return ptr;
}

#define TD_VALID                   (1 << 0)
#define TD_BLOCK                   (0 << 1)
#define TD_TABLE                   (1 << 1)
#define TD_ACCESS                  (1 << 10)
#define TD_KERNEL_PERMS            (1L << 54)
#define TD_INNER_SHARABLE          (3 << 8)

#define TD_KERNEL_TABLE_FLAGS      (TD_TABLE | TD_VALID)
#define TD_KERNEL_BLOCK_FLAGS      (TD_ACCESS | TD_INNER_SHARABLE | TD_KERNEL_PERMS | (MATTR_NORMAL_NC_INDEX << 2) | TD_BLOCK | TD_VALID)
#define TD_DEVICE_BLOCK_FLAGS      (TD_ACCESS | TD_INNER_SHARABLE | TD_KERNEL_PERMS | (MATTR_DEVICE_nGnRnE_INDEX << 2) | TD_BLOCK | TD_VALID)

#define MATTR_DEVICE_nGnRnE        0x0
#define MATTR_NORMAL_NC            0x44
#define MATTR_DEVICE_nGnRnE_INDEX  0
#define MATTR_NORMAL_NC_INDEX      1
#define MAIR_EL1_VAL               ((MATTR_NORMAL_NC << (8 * MATTR_NORMAL_NC_INDEX)) | MATTR_DEVICE_nGnRnE << (8 * MATTR_DEVICE_nGnRnE_INDEX))

#define ID_MAP_PAGES           6
#define ID_MAP_TABLE_SIZE      (ID_MAP_PAGES * PAGE_SIZE)
#define ENTRIES_PER_TABLE      512
#define PGD_SHIFT              (PAGE_SHIFT + 3 * TABLE_SHIFT)
#define PUD_SHIFT              (PAGE_SHIFT + 2 * TABLE_SHIFT)
#define PMD_SHIFT              (PAGE_SHIFT + TABLE_SHIFT)
#define PUD_ENTRY_MAP_SIZE     (1 << PUD_SHIFT)

#define BLOCK_SIZE 0x40000000


void create_table_entry(uint64_t tbl, uint64_t next_tbl, uint64_t va, uint64_t shift, uint64_t flags){
    uint64_t table_index = va >> shift;
    table_index &= (ENTRIES_PER_TABLE - 1);
    uint64_t descriptor = next_tbl | flags;
    *((uint64_t*) (tbl + (table_index << 3))) = descriptor;
}

void create_block_map(uint64_t pmd, uint64_t vstart, uint64_t vend, uint64_t pa){
    vstart >>= SECTION_SHIFT;
    vstart &= (ENTRIES_PER_TABLE - 1);

    vend >>= SECTION_SHIFT;
    vend--;
    vend &= (ENTRIES_PER_TABLE - 1);

    pa >>= SECTION_SHIFT;
    pa <<= SECTION_SHIFT;

    do{
        uint64_t _pa = pa;
        if(pa >= DEVICE_START){
            _pa |= TD_DEVICE_BLOCK_FLAGS;
        }else{
            _pa |= TD_KERNEL_BLOCK_FLAGS;
        }

        *((uint64_t*) (pmd + (vstart << 3))) = _pa;
        pa += SECTION_SIZE;
        vstart++;
    }while(vstart <= vend);
}

uint64_t page_table_base();


void init_mmu(){
    // starts as the L0 page table
    uint64_t id_pgd = page_table_base();

    // zero out next 6 consecutive page tables 
    memzero(id_pgd, ID_MAP_TABLE_SIZE);

    uint64_t map_base = 0;
    uint64_t tbl = id_pgd;
    uint64_t next_tbl = tbl + PAGE_SIZE;

    // create a table entry in the L0 table
    // here, index = 0, descriptor is the next table | TD_KERNEL_TABLE_FLAGS
    create_table_entry(tbl, next_tbl, map_base, PGD_SHIFT, TD_KERNEL_TABLE_FLAGS);

    // slide over to the L1 table we just made
    tbl += PAGE_SIZE;

    // next_tbl = L2 table after the L1 table, 3rd consecutive page table
    next_tbl += PAGE_SIZE;

    // block_tbl = L1 table 
    uint64_t block_tbl = tbl;

    for(int i = 0; i < 4; i++){
        // create a table entry in the L1 table
        // we start at index 0, since map_base = 0 and PUD_SHIFT = 
        create_table_entry(tbl, next_tbl, map_base, PUD_SHIFT, TD_KERNEL_TABLE_FLAGS);

        // next_tbl now points to the next L2 table, starting from the 4th, then 5th, then 6th
        next_tbl += PAGE_SIZE;
        map_base += PUD_ENTRY_MAP_SIZE;
        block_tbl += PAGE_SIZE;
        uint64_t offset = BLOCK_SIZE * i;

        // map virtual to physical
        create_block_map(block_tbl, offset, offset + BLOCK_SIZE, offset);
    }
}