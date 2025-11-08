#include "mmu.h"
#include "mm.h"
#include "virtual_memory.h"
#include "peripherals/base.h"
#include "printf.h"

/*
+===========================+===========================+
|          Address          |          Region           |
+===========================+===========================+
| 0x0                       | RAM Start                 |
+---------------------------+---------------------------+
| 0x8000                    | Kernel code + data        |
+---------------------------+---------------------------+
| __kernel_end              | End of kernel code        |
+---------------------------+---------------------------+
| __kernel_end + KSTACK     | Start of the kernel stack |
+---------------------------+---------------------------+
| __kernel_end + KSTACK + 1 | User addressable memory   |
+---------------------------+---------------------------+
*/

uint64_t mmutest(uint64_t);
void* page_table_base();

// one frame for every physical page in RAM
// page_frame_t frame_metadata[(1 << 20)];

void print_page_table(uint64_t* pt_addr, int n_entries){
    int n = n_entries;
    printf("Dumping top %d entries of page table at address 0x%x:\n", n, pt_addr);
    for(int i = 0; i < n; i++){
        printf("\t%d: 0x%x\n", i, pt_addr[i]);
    }
}


void* _translate(uint64_t address){
    pt_metadata_t* l0_metadata = (pt_metadata_t*) 0x8000;
    uint16_t l0_index = (address >> 39) & 0x1FF;
    uint16_t l1_index = (address >> 30) & 0x1FF;
    uint16_t l2_index = (address >> 21) & 0x1FF;
    uint16_t l3_index = (address >> 12) & 0x1FF;

    table_descriptor_t table_descriptor = {0};
    mem_descriptor_t mem_descriptor = {0};

    PDEBUG("L0 index: %d\n", l0_index);
    PDEBUG("L1 index: %d\n", l1_index);
    PDEBUG("L2 index: %d\n", l2_index);
    PDEBUG("L3 index: %d\n", l3_index);

    PDEBUG("Deconstructing virtual addres...\n");
    uint64_t* l0 = l0_metadata->table_address;
    table_descriptor.value = l0[l0_index];


    if(!table_descriptor.bits.valid){
        PDEBUG("WARNING: Encountered invalid PTE in L0 table at index %d.\n", l0_index);
    }
    uint64_t* l1 = (uint64_t*)((uint64_t) (table_descriptor.bits.address << 12));
    uint64_t l1_pte = l1[l1_index];
    if((l1_pte & 0b11) == 0b11){
        table_descriptor.value = l1_pte;
    }else if(l1_pte & 0b01){
        mem_descriptor.value = l1_pte;
        PDEBUG("Translation complete. Base address: 0x%x\n", (mem_descriptor.bits.address << 12));
        return (void*) ((uint64_t) (mem_descriptor.bits.address << 12));
    }else{
        PDEBUG("WARNING: Encountered invalid PTE in L1 table at index %d.\n", l1_index);
        return 0;
    }

    uint64_t* l2 = (uint64_t*)((uint64_t) (table_descriptor.bits.address << 12));
    uint64_t l2_pte = l2[l2_index];
    PDEBUG("L2 PTE: 0x%x\n", l2_pte);
    if((l2_pte & 0b11) == 0b11){
        table_descriptor.value = l2_pte;
    }else if(l2_pte & 0b01){
        mem_descriptor.value = l2_pte;
        PDEBUG("Translation complete. Base address: 0x%x\n", (mem_descriptor.bits.address << 12));
        return (void*) ((uint64_t) (mem_descriptor.bits.address << 12));
    }else{
        PDEBUG("WARNING: Encountered invalid PTE in L2 table at index %d.\n", l2_index);
        return 0;
    }

    uint64_t* l3 = (uint64_t*)((uint64_t) (table_descriptor.bits.address << 12));
    mem_descriptor.value = l3[l3_index];
    if(!mem_descriptor.bits.valid){
        PDEBUG("WARNING: Encountered invalid PTE in L3 table at index %d.\n", l3_index);
        return 0;
    }
    PDEBUG("Translation complete. Base address: 0x%x\n", (mem_descriptor.bits.address << 12));
    return (void*) ((uint64_t) (mem_descriptor.bits.address << 12));
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


    PDEBUG("Creating L1 table...\n");
    // page table descriptor
    for(int i = 0; i < 4; i++){
        // move pointer to next l2 table
        l2_page_table = UNSCALED_POINTER_ADD(l2_page_table, PAGE_TABLE_SIZE);

        // update address to that table
        l1_table_descriptor.bits.address = ((uint64_t)l2_page_table) >> 12;

        // write new entry into the L1 table
        put64((uint64_t) ((uint64_t*) l1_page_table + i), l1_table_descriptor.value);
    }

    PDEBUG("Populating L2 tables...\n");
    // --> addr = 0, ng = 1, af = 1, sh = 11, ap = 00, ns = 0, attr_index = 001, type = 0, valid = 1
    // this block maps the first 2MiB: 0x0-0x40000000
    mem_descriptor_t l2_block_descriptor = {0};
    l2_block_descriptor.bits.af = 1;
    l2_block_descriptor.bits.sh = 3;
    l2_block_descriptor.bits.ap = 0;
    l2_block_descriptor.bits.ns = 0;
    l2_block_descriptor.bits.attr_index = 1;
    l2_block_descriptor.bits.uxn = 0;
    l2_block_descriptor.bits.pxn = 0;
    l2_block_descriptor.bits.type = 0;
    l2_block_descriptor.bits.valid = 1;


    l2_page_table = (uint64_t*) UNSCALED_POINTER_ADD(l1_page_table, PAGE_TABLE_SIZE);
    uint64_t base_addr = 0;
    for(int i = 0; i < 4 * 512; i++){
        l2_block_descriptor.bits.address = (base_addr >> 12);
        if (base_addr >= PBASE){
            l2_block_descriptor.bits.attr_index = 0;
            l2_block_descriptor.bits.uxn = 1;
            l2_block_descriptor.bits.pxn = 1;
        }
        put64((uint64_t) ((uint64_t*) l2_page_table + i), l2_block_descriptor.value);
        base_addr += (1 << 21);
    }
}

void create_user_mapping(pt_metadata_t* pt0_metadata){
    // designate 4 blocks of memory (10MiB to the user stack)
    // it goes from L0[0] -> L1[8]
    uint16_t l0_index = (USTACK >> 39) & 0x1FF;
    uint16_t l1_index = (USTACK >> 30) & 0x1FF;
    uint16_t l2_index = (USTACK >> 21) & 0x1FF;

    uint64_t* l0_page_table = (uint64_t*) pt0_metadata->table_address;
    table_descriptor_t l0_pte = {l0_page_table[l0_index]};
    uint64_t* l1_page_table = (uint64_t*) ((uint64_t) (l0_pte.bits.address << 12));

    // 0x10000 -> L0
    // 0x11000 -> L1
    // 0x12000 -> L2 #1
    // 0x13000 -> L2 #2
    // 0x14000 -> L2 #3
    // 0x15000 -> L2 #4
    // 0x16000 -> NEW L2
    uint64_t* new_l2_page_table = (uint64_t*) 0x16000;

    // zero out new page table
    memset(new_l2_page_table, 8 * 512, 0);

    table_descriptor_t l1_pte = {0};
    l1_pte.bits.valid = 1;
    l1_pte.bits.type = 1;
    l1_pte.bits.address = (uint64_t) new_l2_page_table >> 12;
    put64((uint64_t) (l1_page_table + l1_index), l1_pte.value);

    mem_descriptor_t l2_block_descriptor = {0};
    l2_block_descriptor.bits.af = 1;
    l2_block_descriptor.bits.sh = 3;
    l2_block_descriptor.bits.ap = EL0_RW_EL1_RW;
    l2_block_descriptor.bits.ns = 0;
    l2_block_descriptor.bits.attr_index = 1;
    l2_block_descriptor.bits.uxn = 0;
    l2_block_descriptor.bits.pxn = 0;
    l2_block_descriptor.bits.type = 0;
    l2_block_descriptor.bits.valid = 1;
    
    // create 4 entries for the user stack -> 8 MiB stack
    uint64_t base_addr = ((USTACK >> 21) << 21);
    for(int i = 0; i < 4; i++){
        l2_block_descriptor.bits.address = base_addr >> 12;
        put64((uint64_t) ((uint64_t*) (new_l2_page_table + l2_index - i)), l2_block_descriptor.value);
        base_addr -= (1 << 21);
    }



    // at this moment, we have 6 page tables alloated
    // now, we need to allocate 1 more


}

void map_page(uint64_t* pt, uint64_t virt, uint64_t phys, int flags){

}

void unmap_page(uint64_t* pt, uint64_t virt){

}


void initialize_page_tables(void* ptb, pt_metadata_t* pt_metadata_start){
    // first, invalidate all metadata in the region
    PDEBUG("\tInitializing page tables...\n");
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
    create_user_mapping(pt_metadata_start);
    // should both be 0xb00
    // PDEBUG("MMU Test with 0x0: 0x%x\n", mmutest(0x0));
    // PDEBUG("MMU Test with 0x100000: 0x%x\n", mmutest(0x100000));


    // print_page_table((uint64_t*) page_table_base(), 5);
    // print_page_table((uint64_t*) ((uint64_t)page_table_base() + (1 * PAGE_SIZE)), 5);
    // print_page_table((uint64_t*) ((uint64_t)page_table_base() + (2 * PAGE_SIZE)), 25);
    // print_page_table((uint64_t*) ((uint64_t)page_table_base() + (3 * PAGE_SIZE)), 25);
    // print_page_table((uint64_t*) ((uint64_t)page_table_base() + (4 * PAGE_SIZE)), 25);
    // print_page_table((uint64_t*) ((uint64_t)page_table_base() + (5 * PAGE_SIZE)), 25);
    // print_page_table((uint64_t*) ((uint64_t)page_table_base() + (6 * PAGE_SIZE)), 512);


    // void* result;
    // result = _translate(0x100000);
    // PDEBUG("Translation result of user memory: 0x%x\n", result);
    // result = _translate(0x0FFFFF);
    // PDEBUG("Translation result of user memory - 1: 0x%x\n", result);
    // result = _translate(PBASE);
    // PDEBUG("Translation result of peripheral base: 0x%x\n", result);
    // result = _translate(0x100000);
    // PDEBUG("Translation result of user code segment: 0x%x\n", result);


    // PDEBUG("\tTable initialization complete. Enabling MMU...\n");
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