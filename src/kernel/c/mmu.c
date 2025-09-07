#include "mmu.h"

void initialize_page_tables(void* page_table_base){
    table_descriptor_t l1_pte = {0};

    // mark the l1_pte as valid, and as a table descriptor
    l1_pte.bits.valid = 1;
    l1_pte.bits.type = 1;

    // slide over 4KiB in memory to the level 2 page table
    uint64_t l2_page_table_addr = (uint64_t) ((char*) page_table_base + (1 << 12));

    // point to the next level page table
    l1_pte.bits.address = (l2_page_table_addr >> 12) & 0xFFFFFFFFF;

    // create a page table descriptor for level 2
    page_descriptor_t l2_pte = {0};

    l2_pte.bits.valid = 1;
    l2_pte.bits.type = 1; // 1 means it's a page descriptor
    // l2_pte.bits.address = 


    put64((uint64_t) page_table_base, l1_pte.value);
    put64((uint64_t) l2_page_table_addr, l2_pte.value);
}