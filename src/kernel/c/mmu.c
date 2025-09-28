#include "mmu.h"
#include "virtual_memory.h"
#include "peripherals/base.h"
#include "printf.h"

/*
b create_kernel_identity_mapping; b create_peripheral_identity_mapping; b initialize_page_tables
*/


void mmutest();

void* _translate(uint64_t address, pt_metadata_t* l0_metadata){
    uint16_t l0_index = (address >> 39) & 0x1FF;
    uint16_t l1_index = (address >> 30) & 0x1FF;

    printf("L0 index: %d\n", l0_index);
    printf("L1 index: %d\n", l1_index);

    printf("Deconstructing virtual addres...\n");
    uint64_t* l0 = l0_metadata->table_address;

    table_descriptor_t l1 = {0};
    l1.value = l0[l0_index];

    printf("L0 entry: 0x%x\n", l1.value);

    uint64_t* l1_address = (uint64_t*) (uint64_t)(l1.bits.address << 12);
    
    mem_descriptor_t l1_entry = {0};
    l1_entry.value = l1_address[l1_index];

    printf("L1 entry: 0x%x\n", l1_entry.value);
    printf("L1 block address: 0x%x\n", l1_entry.bits.address);
    uint64_t phys_addr = (l1_entry.bits.address << 12) | (address & ((1 << 30) - 1));
    printf("Full physical address: 0x%x\n", phys_addr);

    return (void*) phys_addr;
}

void create_kernel_identity_mapping(pt_metadata_t* pt0_metadata){
    // at this point, the l0 metadata should be populated

    // next, we need to identity map the first 256 MiB of RAM
    // for early development, we can just identity map the first full GiB using the L1 table
    // this is UNSAFE and should be changed later, using finer granularity for page mapping

    // add an entry to the L0 page table, denoting the location of the L1 page table
    void* l0_page_table = pt0_metadata->table_address;

    // L1 page table data
    void* l1_page_table = UNSCALED_POINTER_ADD(l0_page_table, PAGE_TABLE_SIZE);
    pt_metadata_t* l1_metadata = UNSCALED_POINTER_ADD(pt0_metadata, sizeof(pt_metadata_t));

    table_descriptor_t l0_table_descriptor = {0};
    l0_table_descriptor.bits.valid = 1;
    l0_table_descriptor.bits.type = 1;   // indicates this is a pointer to next-level table
    l0_table_descriptor.bits.address = ((uint64_t)l1_page_table) >> 12;

    put64((uint64_t) l0_page_table, l0_table_descriptor.value);
    pt0_metadata->count++;


    // locate the metadata for the first L1 page table, and mark it as valid with 1 entry (to be added)
    l1_metadata->index = 0;
    l1_metadata->count = 1;
    l1_metadata->level = PT_LEVEL1;
    l1_metadata->table_address = l1_page_table;
    l1_metadata->next = NULL;

    // create the 1 GiB block descriptor
    mem_descriptor_t block_descriptor = {0};
    block_descriptor.bits.valid = 1;        // mark as valid
    block_descriptor.bits.type = 0;         // at l1, 0 is a 1GiB block mapping
    block_descriptor.bits.attr_index = 0;   // TBD
    block_descriptor.bits.ap = 0b11;        // 
    block_descriptor.bits.sh = 0b11;        // 0b11 is inner shareable
    block_descriptor.bits.af = 1;           // set access flag
    block_descriptor.bits.ng = 1;           // set non-global
    block_descriptor.bits.address = 0x0;    // start map with physical address 0
    block_descriptor.bits.uxn = 0;          // allow user to execute in this range (DANGEROUS!!!)
    block_descriptor.bits.pxn = 0;          // allow higher privelage (kernel) to execute

    // add the entry to the page table
    put64((uint64_t) l1_page_table, block_descriptor.value);

}

void create_peripheral_identity_mapping(pt_metadata_t* l0_page_table_metadata){
    // we now need to map th 3rd index of the L1 page table to address base 0xC0000000
    pt_metadata_t* l1_metadata = UNSCALED_POINTER_ADD(l0_page_table_metadata, sizeof(pt_metadata_t));
    void* l1_page_table = l1_metadata->table_address;
    uint64_t* entry3 = (uint64_t*) l1_page_table + 3;

    mem_descriptor_t block_descriptor = {0};
    block_descriptor.bits.valid = 1;        // mark as valid
    block_descriptor.bits.type = 0;         // at l1, 0 is a 1GiB block mapping
    block_descriptor.bits.attr_index = 0;   // TBD
    block_descriptor.bits.ap = 0b00;        // access permission of 0b00 means El1 RW, EL0 no access
    block_descriptor.bits.sh = 0b11;        // 0b11 is inner shareable
    block_descriptor.bits.af = 1;           // set access flag
    block_descriptor.bits.ng = 1;           // set non-global

    // the base for the 1GiB block containing the peripherals is 0xC0000000
    block_descriptor.bits.address = 0xC0000000 >> 12;
    block_descriptor.bits.uxn = 1;          // don't allow user to execute
    block_descriptor.bits.pxn = 1;          // don't allow kernel to execute

    put64((uint64_t) entry3, block_descriptor.value);
}

// not used for early kernel development
// void create_user_mapping(void* l0_page_table){

// }

// not used for early kernel development
// void _alloc_pt_metadata(pt_level_t level){

// }


void initialize_page_tables(void* page_table_base, pt_metadata_t* pt_metadata_start){
    // first, invalidate all metadata in the region
    printf("\tInitializing page tables...\n");
    memset(pt_metadata_start, sizeof(pt_metadata_t) * 513, INVALID_PT_METADATA);

    // set up the page table metadata 
    // there's only 1 L0 table, so the next bytes are read as that
    // there are 512 L1 page tables, so the next 512 correspond to the indices in the L0 table
    // since there are 512^2 L2 page tables, we use a linked list structure to define those
    // same with L3, since there are 512^3 possible L3 page tables
    pt_metadata_t* l0_page_table_metadata = pt_metadata_start;
    l0_page_table_metadata->index = 0;
    l0_page_table_metadata->count = 0;
    l0_page_table_metadata->level = PT_LEVEL0;
    l0_page_table_metadata->table_address = page_table_base;
    l0_page_table_metadata->next = NULL;

    create_kernel_identity_mapping(pt_metadata_start);
    // create_peripheral_identity_mapping(pt_metadata_start);
    mmutest();

    printf("\tTable initialization complete. Enabling MMU...\n");
}

// first PTE address ranges from 0x0 - 0x200 (which is 0x40000000 >> 21)
// 1 GiB = 1 << 30 = 
// = 0x40000000
// = 0xFE000000

// Full mapping:
// 0x00000000 --> 0x0
// 0x40000000 --> 0x1
// 0x80000000 --> 0x2
// 0xC0000000 --> 0x3