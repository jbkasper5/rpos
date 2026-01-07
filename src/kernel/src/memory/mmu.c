#include "memory/mmu.h"
#include "memory/mm.h"
#include "memory/mmap.h"
#include "memory/virtual_memory.h"
#include "memory/paging.h"
#include "io/lcd.h"
#include "io/gpio.h"
#include "peripherals/base.h"

#include "io/printf.h"

uint64_t* L0_TABLE;


uint64_t mmutest(uint64_t);

uint64_t translate_va(uint64_t va, uint64_t ttbr1_base) {
    uint64_t *l0 = (uint64_t *)(ttbr1_base & ~0xFFFUL);

    int idx0 = (va >> 39) & 0x1FF;
    int idx1 = (va >> 30) & 0x1FF;
    int idx2 = (va >> 21) & 0x1FF;
    int idx3 = (va >> 12) & 0x1FF;
    int page_off = va & 0xFFF;

    table_descriptor_t d0 = (table_descriptor_t) l0[idx0];
    if (!(d0.bits.valid)) return 0; // Fault
    uint64_t* l1 = (uint64_t*)(d0.bits.address << 12);

    table_descriptor_t d1 = (table_descriptor_t) l1[idx1];
    if (!(d1.bits.valid)) return 0;
    if (d1.bits.type == 0) {
        // 1GB block
        uint64_t pa = (d1.bits.address << 30) + (va & 0x3FFFFFFF);
        return pa;
    }
    uint64_t *l2 = (uint64_t *)(d1.bits.address << 12);

    table_descriptor_t d2 = (table_descriptor_t) l2[idx2];
    if (!(d2.bits.valid)) return 0;
    if (d2.bits.type == 0) {
        // 2MB block
        uint64_t pa = (d2.bits.address << 12) + (va & 0x1FFFFF);
        return pa;
    }
    uint64_t *l3 = (uint64_t *)(d2.bits.address << 12);

    mem_descriptor_t d3 = (mem_descriptor_t) l3[idx3];
    if (!(d3.bits.valid)) return 0;

    uint64_t raw_pa = (d3.bits.address << 12) | page_off;
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
    table_descriptor_t* kernel_l0_pt = (table_descriptor_t*) buddy_alloc_pt();

    L0_TABLE = (uint64_t*) kernel_l0_pt;

    uint64_t order = log2_pow2(allocated_pages);
    uint64_t flags = MAP_KERNEL | MAP_EXEC;
    map(0x0, 0x0, order, flags, (uint64_t) L0_TABLE);


    // peripheral addresses occupy a total of 16MiB, or 8 L2-blocks (each 2MiB)
    flags = MAP_DEVICE | MAP_KERNEL;
    map(PBASE, PBASE, 12, flags, (uint64_t) L0_TABLE);

    return (uint64_t*) kernel_l0_pt;
}


uint64_t* initialize_page_tables(){
    uint64_t allocated_pages = initialize_page_frame_array();
    uint64_t* l0_pt = create_kernel_identity_mapping(allocated_pages);
    map_pages(frame.fb, frame.fb, 376, MAP_KERNEL, (uint64_t) L0_TABLE);

    INFO("Translated PBASE: 0x%x\n", translate_va(PBASE, L0_TABLE));
    INFO("Translated AUX: 0x%x\n", translate_va(REGS_AUX, L0_TABLE));
    INFO("Walk result: 0x%x\n", walk(PBASE));
    INFO("Walk result: 0x%x\n", walk(REGS_AUX));
    return l0_pt;
}