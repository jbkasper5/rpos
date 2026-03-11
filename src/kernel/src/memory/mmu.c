#include "memory/mmu.h"
#include "memory/mm.h"
#include "memory/mmap.h"
#include "memory/virtual_memory.h"
#include "memory/paging.h"
#include "io/lcd.h"
#include "io/gpio.h"
#include "peripherals/base.h"
#include "peripherals/gic.h"

#include "io/kprintf.h"
#include "asm_utils.h"

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
    if (!(d0.bits.valid)) return -1; // Fault
    uint64_t* l1 = (uint64_t*)(d0.bits.address << 12);

    table_descriptor_t d1 = (table_descriptor_t) l1[idx1];
    if (!(d1.bits.valid)) return -1;
    if (d1.bits.type == 0) {
        // 1GB block
        uint64_t pa = (d1.bits.address << 30) + (va & 0x3FFFFFFF);
        return pa;
    }
    uint64_t *l2 = (uint64_t *)(d1.bits.address << 12);

    table_descriptor_t d2 = (table_descriptor_t) l2[idx2];
    if (!(d2.bits.valid)) return -1;
    if (d2.bits.type == 0) {
        // 2MB block
        uint64_t pa = (d2.bits.address << 12) + (va & 0x1FFFFF);
        return pa;
    }
    uint64_t *l3 = (uint64_t *)(d2.bits.address << 12);

    mem_descriptor_t d3 = (mem_descriptor_t) l3[idx3];
    if (!(d3.bits.valid)) return -1;

    uint64_t raw_pa = (d3.bits.address << 12) | page_off;
    return raw_pa & ((1ULL << 36) - 1);   // Pi4 36-bit PA mask
}


void print_page_table(uint64_t* pt_addr, int n_entries){
    int n = n_entries;
    kprintf("Dumping top %d entries of page table at address 0x%x:\n", n, pt_addr);
    for(int i = 0; i < n; i++){
        kprintf("\t%d: 0x%x\n", i, pt_addr[i]);
    }
}

static void map_page_frame_array(){
    uint64_t start_addr = ALIGN_DOWN(page_frame_array_start(), PAGE_SIZE);
    uint64_t end_addr = ALIGN_UP(page_frame_array_end(), PAGE_SIZE);

    uint64_t pages_to_map = (end_addr - start_addr) >> 12;

    uint64_t next_block_addr = ALIGN_UP(start_addr, BLOCK_SIZE);

    uint64_t frontside_pages = (next_block_addr - start_addr) >> 12;
    
    INFO("Mapping %d frontside pages at address 0x%x\n", frontside_pages, start_addr);

    map_pages(start_addr, va_to_pa(start_addr), frontside_pages, MAP_KERNEL, L0_TABLE);

    uint64_t n_blocks = (pages_to_map - frontside_pages) / 512;

    start_addr += (frontside_pages << PAGE_SHIFT);

    INFO("Mapping %d blocks at address 0x%x\n", n_blocks, start_addr);

    map_blocks(start_addr, va_to_pa(start_addr), n_blocks, MAP_KERNEL, L0_TABLE);

    uint64_t backside_pages = pages_to_map - frontside_pages - (512 * n_blocks);

    start_addr += (n_blocks << BLOCK_BITS);

    INFO("Mapping %d backside pages at address 0x%x\n", backside_pages, start_addr);

    map_pages(start_addr, va_to_pa(start_addr), backside_pages, MAP_KERNEL, L0_TABLE);
}


static void map_static_page_region(){
    uintptr_t start_addr = static_page_region_start();
    map_pages(start_addr, start_addr, static_page_region_pages(), MAP_KERNEL, L0_TABLE);
}

uint64_t* finish_virtual_mapping(){
    L0_TABLE = static_page_region_start();

    uint64_t flags = MAP_KERNEL | MAP_DEVICE;
    map(PBASE, va_to_pa(PBASE), 12, flags, (uint64_t) L0_TABLE);

    uint64_t gic_virtual = ALIGN_DOWN(GIC_BASE, BLOCK_SIZE);
    map(gic_virtual, va_to_pa(gic_virtual), 9, flags, L0_TABLE);

    // map the page frame array metadata detailing RAM
    map_page_frame_array();

    // map the page tables themselves
    map_static_page_region();

    // initialize the buddy allocator
    initialize_page_frame_array();

    // return the L0 table to be stored in TTBR_EL1
    return L0_TABLE;
}

// 765 608 6680