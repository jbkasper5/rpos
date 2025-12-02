#include "mmu.h"
#include "mm.h"
#include "virtual_memory.h"
#include "peripherals/base.h"
#include "printf.h"
#include "paging.h"

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

table_descriptor_t kernel_table_descriptor = {
    .bits.valid = 1,
    .bits.type = 1,
    .bits.address = 0
};

mem_descriptor_t kernel_block_descriptor = {
    .bits.af = 1,
    .bits.sh = 3,
    .bits.ap = 0,
    .bits.ns = 0,
    .bits.attr_index = 1,
    .bits.uxn = 1,
    .bits.pxn = 0,
    .bits.type = 0,
    .bits.valid = 1,
};

mem_descriptor_t kernel_page_descriptor = {
    .bits.af = 1,
    .bits.sh = 3,
    .bits.ap = 0,
    .bits.ns = 0,
    .bits.attr_index = 1,
    .bits.uxn = 1,
    .bits.pxn = 0,
    .bits.type = 1,
    .bits.valid = 1,
};

mem_descriptor_t kernel_mmio_block_descriptor = {
    .bits.af = 1,
    .bits.sh = 3,
    .bits.ap = 0,
    .bits.ns = 0,
    .bits.attr_index = 0,
    .bits.uxn = 1,
    .bits.pxn = 1,
    .bits.type = 0,
    .bits.valid = 1,
};


uint64_t translate_va(uint64_t va, uint64_t ttbr1_base) {
    uint64_t *l0 = (uint64_t *)(ttbr1_base & ~0xFFFUL);

    int idx0 = (va >> 39) & 0x1FF;
    int idx1 = (va >> 30) & 0x1FF;
    int idx2 = (va >> 21) & 0x1FF;
    int idx3 = (va >> 12) & 0x1FF;
    int page_off = va & 0xFFF;

    uint64_t d0 = l0[idx0];
    if (!(d0 & 1)) return 0; // Fault
    uint64_t *l1 = (uint64_t *)((d0 >> 12) << 12);

    uint64_t d1 = l1[idx1];
    if (!(d1 & 1)) return 0;
    if ((d1 & 0b11) == 0b01) {
        // 1GB block
        uint64_t pa = (d1 & ~0x3FFFFFFF) + (va & 0x3FFFFFFF);
        return pa;
    }
    uint64_t *l2 = (uint64_t *)((d1 >> 12) << 12);

    uint64_t d2 = l2[idx2];
    if (!(d2 & 1)) return 0;
    if ((d2 & 0b11) == 0b01) {
        // 2MB block
        uint64_t pa = (d2 & ~0x1FFFFF) + (va & 0x1FFFFF);
        return pa;
    }
    uint64_t *l3 = (uint64_t *)((d2 >> 12) << 12);

    uint64_t d3 = l3[idx3];
    if (!(d3 & 1)) return 0;

    uint64_t raw_pa = (d3 & ~0xFFFUL) | page_off;
    return raw_pa & ((1ULL << 36) - 1);   // Pi4 36-bit PA mask
}


void print_page_table(uint64_t* pt_addr, int n_entries){
    int n = n_entries;
    printf("Dumping top %d entries of page table at address 0x%x:\n", n, pt_addr);
    for(int i = 0; i < n; i++){
        printf("\t%d: 0x%x\n", i, pt_addr[i]);
    }
}

uint64_t* create_kernel_identity_mapping(uint64_t allocated_pages){
    /*
    Once page frame array is populated, we can allocate pages and create 
    the page table mapping for the kernel memory
    */

    // allocate 4 pages for the kernels initial page table structure
    table_descriptor_t* kernel_l0_pt = (table_descriptor_t*) buddy_alloc(PAGE_SIZE);
    table_descriptor_t* kernel_l1_pt = (table_descriptor_t*) buddy_alloc(PAGE_SIZE);
    table_descriptor_t* kernel_l2_pt = (table_descriptor_t*) buddy_alloc(PAGE_SIZE);
    mem_descriptor_t* kernel_l3_pt = (mem_descriptor_t*) buddy_alloc(PAGE_SIZE);

    PDEBUG("L0 PT: 0x%x\n", kernel_l0_pt);
    PDEBUG("L1 PT: 0x%x\n", kernel_l1_pt);
    PDEBUG("L2 PT: 0x%x\n", kernel_l2_pt);
    PDEBUG("L3 PT: 0x%x\n", kernel_l3_pt);

    table_descriptor_t l0_descriptor = kernel_table_descriptor;
    l0_descriptor.bits.address = (uint64_t) kernel_l1_pt >> 12;
    put64((uint64_t) kernel_l0_pt, l0_descriptor.value);

    table_descriptor_t l1_descriptor = kernel_table_descriptor;
    l1_descriptor.bits.address = (uint64_t) kernel_l2_pt >> 12;
    put64((uint64_t) kernel_l1_pt, l1_descriptor.value);


    table_descriptor_t l2_descriptor = kernel_table_descriptor;
    l2_descriptor.bits.address = (uint64_t) kernel_l3_pt >> 12;
    put64((uint64_t) kernel_l2_pt, l2_descriptor.value);


    // need to create L0, L1, L2 tables
    // L3 maps 4KiB pages
    // L2 maps 2MiB blocks
    // L1 maps 1GiB blocks
    // L0 would map 512 GiB blocks

    mem_descriptor_t l3_descriptor = kernel_page_descriptor;
    // skip past the first page to leave it unmapped, helps 
    // guard against silent NULL pointer dereferences
    for(int pfn = 1; pfn < allocated_pages; pfn++){
        l3_descriptor.bits.address = pfn;
        put64((uint64_t) (kernel_l3_pt + pfn), l3_descriptor.value);
    }

    // allocate a second l2 page table for the peripheral MMIO block mappings
    mem_descriptor_t* mmio_l2_table = (mem_descriptor_t*) buddy_alloc(PAGE_SIZE);

    // get the page table indices where the mmio peripheral base would lie
    uint64_t l1_mmio_index = (PBASE >> 30) & 0x1FF;
    uint64_t l2_mmio_index = (PBASE >> 21) & 0x1FF;

    // add the new l2 table into the existing VM structure starting from appropriate offset
    l1_descriptor.bits.address = (uint64_t) mmio_l2_table >> 12;
    put64((uint64_t) (kernel_l1_pt + l1_mmio_index), l1_descriptor.value);
    mem_descriptor_t mmio_descriptor = kernel_mmio_block_descriptor;
    mmio_descriptor.bits.address = (PBASE >> 12);

    // peripheral addresses occupy a total of 16MiB, or 8 L2-blocks (each 2MiB)
    for(int i = 0; i < 8; i++){
        mmio_descriptor.bits.address = (PBASE >> 12) + (i * 512);
        put64((uint64_t) (mmio_l2_table + l2_mmio_index + i), mmio_descriptor.value);
    }

    // uint64_t pa = translate_va((uint64_t) KSTACK, (uint64_t) kernel_l0_pt);
    // PDEBUG("PA: 0x%x\n", pa);

    return (uint64_t*) kernel_l0_pt;
}


uint64_t* initialize_page_tables(void* ptb, pt_metadata_t* pt_metadata_start){
    uint64_t allocated_pages = initialize_page_frame_array();
    return create_kernel_identity_mapping(allocated_pages);
}