#include "mmu.h"
#include "virtual_memory.h"
#include "peripherals/base.h"
#include "printf.h"

/*
b create_kernel_identity_mapping; b create_peripheral_identity_mapping; b initialize_page_tables
*/

uint64_t mmutest(uint64_t);
void* page_table_base();

void print_page_table(uint64_t* pt_addr, int n_entries){
    int n = n_entries;
    printf("Dumping top %d entries of page table at address 0x%x:\n", n, pt_addr);
    for(int i = 0; i < n; i++){
        printf("\t%d: 0x%x\n", i, pt_addr[i]);
    }
}


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
    // pt_metadata_t* l1_metadata = UNSCALED_POINTER_ADD(pt0_metadata, sizeof(pt_metadata_t));

    table_descriptor_t l0_table_descriptor = {0};
    l0_table_descriptor.bits.valid = 1;
    l0_table_descriptor.bits.type = 1;   // indicates this is a pointer to next-level table
    l0_table_descriptor.bits.address = ((uint64_t)l1_page_table) >> 12;

    put64((uint64_t) l0_page_table, l0_table_descriptor.value);
    pt0_metadata->count++;


    // add an entry for the first L2 page table in the list
    void* l2_page_table = l1_page_table;
    table_descriptor_t l1_table_descriptor = {0};
    l1_table_descriptor.bits.valid = 1;
    l1_table_descriptor.bits.type = 1;


    printf("Creating L1 table...\n");
    // page table descriptor
    for(int i = 0; i < 4; i++){
        // move pointer to next l2 table
        l2_page_table = UNSCALED_POINTER_ADD(l2_page_table, PAGE_TABLE_SIZE);

        // update address to that table
        l1_table_descriptor.bits.address = ((uint64_t)l2_page_table) >> 12;

        // write new entry into the L1 table
        put64((uint64_t) ((uint64_t*) l1_page_table + i), l1_table_descriptor.value);
    }

    printf("Populating L2 tables...\n");
    // --> addr = 0, ng = 1, af = 1, sh = 11, ap = 00, ns = 0, attr_index = 001, type = 0, valid = 1
    // this block maps the first 2MiB: 0x0-0x40000000
    mem_descriptor_t l2_block_descriptor = {0};
    l2_block_descriptor.bits.af = 1;
    l2_block_descriptor.bits.sh = 3;
    l2_block_descriptor.bits.ap = 0;
    l2_block_descriptor.bits.ns = 0;
    l2_block_descriptor.bits.attr_index = 1;
    l2_block_descriptor.bits.type = 0;
    l2_block_descriptor.bits.valid = 1;


    l2_page_table = (uint64_t*) UNSCALED_POINTER_ADD(l1_page_table, PAGE_TABLE_SIZE);
    uint64_t base_addr = 0;
    for(int i = 0; i < 4 * 512; i++){
        l2_block_descriptor.bits.address = (base_addr >> 12);
        put64((uint64_t) ((uint64_t*) l2_page_table + i), l2_block_descriptor.value);
        base_addr += (1 << 21);
    }
}

void create_peripheral_identity_mapping(pt_metadata_t* l0_page_table_metadata){

}

// not used for early kernel development
// void create_user_mapping(void* l0_page_table){

// }

// not used for early kernel development
// void _alloc_pt_metadata(pt_level_t level){

// }


void initialize_page_tables(void* ptb, pt_metadata_t* pt_metadata_start){
    // first, invalidate all metadata in the region
    printf("\tInitializing page tables...\n");
    memset(pt_metadata_start, sizeof(pt_metadata_t) * 513, INVALID_PT_METADATA);
    memset(ptb, 6 * 8 * 512, 0);

    // set up the page table metadata 
    // there's only 1 L0 table, so the next bytes are read as that
    // there are 512 L1 page tables, so the next 512 correspond to the indices in the L0 table
    // since there are 512^2 L2 page tables, we use a linked list structure to define those
    // same with L3, since there are 512^3 possible L3 page tables
    pt_metadata_t* l0_page_table_metadata = pt_metadata_start;
    l0_page_table_metadata->index = 0;
    l0_page_table_metadata->count = 0;
    l0_page_table_metadata->level = PT_LEVEL0;
    l0_page_table_metadata->table_address = ptb;
    l0_page_table_metadata->next = NULL;

    create_kernel_identity_mapping(pt_metadata_start);
    // create_peripheral_identity_mapping(pt_metadata_start);
    
    // should both be 0xb00
    printf("MMU Test with 0x0: 0x%x\n", mmutest(0x0));
    printf("MMU Test with 0x100000: 0x%x\n", mmutest(0x100000));


    print_page_table((uint64_t*) page_table_base(), 5);
    print_page_table((uint64_t*) ((uint64_t)page_table_base() + (1 * PAGE_SIZE)), 5);
    print_page_table((uint64_t*) ((uint64_t)page_table_base() + (2 * PAGE_SIZE)), 25);
    print_page_table((uint64_t*) ((uint64_t)page_table_base() + (3 * PAGE_SIZE)), 25);
    print_page_table((uint64_t*) ((uint64_t)page_table_base() + (4 * PAGE_SIZE)), 25);
    print_page_table((uint64_t*) ((uint64_t)page_table_base() + (5 * PAGE_SIZE)), 25);


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