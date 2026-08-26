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

#include "types/paging_types.h"

u64* L0_TABLE;

u64 mmutest(u64);

u64 translate_va(u64 va, u64 ttbr1_base) {
    u64 *l0 = (u64 *)(ttbr1_base & ~0xFFFUL);

    int idx0 = (va >> 39) & 0x1FF;
    int idx1 = (va >> 30) & 0x1FF;
    int idx2 = (va >> 21) & 0x1FF;
    int idx3 = (va >> 12) & 0x1FF;
    int page_off = va & 0xFFF;

    table_descriptor_t d0 = (table_descriptor_t) l0[idx0];
    if (!(d0.bits.valid)) return -1; // Fault
    u64* l1 = (u64*)((u64) d0.bits.address << 12);

    table_descriptor_t d1 = (table_descriptor_t) l1[idx1];
    if (!(d1.bits.valid)) return -1;
    if (d1.bits.type == 0) {
        // 1GB block
        u64 pa = (d1.bits.address << 30) + (va & 0x3FFFFFFF);
        return pa;
    }
    u64 *l2 = (u64*)((u64) d1.bits.address << 12);

    table_descriptor_t d2 = (table_descriptor_t) l2[idx2];
    if (!(d2.bits.valid)) return -1;
    if (d2.bits.type == 0) {
        // 2MB block
        u64 pa = (d2.bits.address << 12) + (va & 0x1FFFFF);
        return pa;
    }
    u64 *l3 = (u64*)((u64) d2.bits.address << 12);

    mem_descriptor_t d3 = (mem_descriptor_t) l3[idx3];
    if (!(d3.bits.valid)) return -1;

    u64 raw_pa = (d3.bits.address << 12) | page_off;
    return raw_pa & ((1ULL << 36) - 1);   // Pi4 36-bit PA mask
}


void print_page_table(u64* pt_addr, int n_entries){
    int n = n_entries;
    kprintf("Dumping top %d entries of page table at address 0x%x:\n", n, pt_addr);
    for(int i = 0; i < n; i++){
        kprintf("\t%d: 0x%x\n", i, pt_addr[i]);
    }
}

u64* finish_virtual_mapping(){
    // initialize the buddy allocator
    initialize_page_frame_array();

    // u64 low_table = READ_SYSREG(ttbr0_el1);
// TODO: free the low table
    
    L0_TABLE = (u64*) pa_to_va(READ_SYSREG(ttbr1_el1));

    // map the peripherals
    u64 flags = MAP_KERNEL | MAP_DEVICE | MAP_WRITE | MAP_READ;
    map(PBASE, va_to_pa(PBASE), 12, flags, (u64) L0_TABLE);

    // map GIC peripherals
    u64 gic_virtual = ALIGN_DOWN(GIC_BASE, BLOCK_SIZE);
    map(gic_virtual, va_to_pa(gic_virtual), 9, flags, (u64) L0_TABLE);

    // map the page frame array metadata detailing RAM
    // map_page_frame_array();

    // return the L0 table to be stored in TTBR_EL1
    return L0_TABLE;
}

// 765 608 6680