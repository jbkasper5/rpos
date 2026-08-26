#include "macros.h"
#include "memory/mmap.h"
#include "linker_symbols.h"

extern u64 static_page_region_pages();
extern u64 static_page_region_start();
extern u64 static_page_region_end();
extern u64 static_page_region_start_phys();

extern u64 virt_base_lo();
extern u64 kernel_high_start();
extern u64 kernel_high_end();
extern u64 kernel_start();
extern u64 get_bss_end();

extern void enable_mmu();

static BOOT_BSS pte k_code_table_mapping = {0};
static BOOT_BSS pte k_code_mem_mapping = {0};
static BOOT_BSS int lo_allocated_pages = 0;

static BOOT_BSS enum permissions{
    RO,
    RW,
    RX
};


static BOOT_FN u64 alloc_page_table_lo(){
    // get physical address of the allocated pages variable
    // int* var = UNSCALED_POINTER_SUB(&allocated_pages, virt_base_lo());
    u64* page_addr = (u64*) UNSCALED_POINTER_ADD(static_page_region_start_phys(), (PAGE_SIZE * lo_allocated_pages));
    lo_allocated_pages++;
    for(int i = 0; i < PAGE_SIZE / sizeof(u64); i++) page_addr[i] = 0;
    return (u64) page_addr;
}


// ––––––––––––––– MAPPING HELPERS ––––––––––––––– //
static BOOT_FN void map_l3_range(u64 va, u64 pa, int l3_blocks, u64* pt, enum permissions perms){
    u32 idx0, idx1, idx2, idx3;

    pte* l0_table = (pte*) pt;
    pte* l1_table, *l2_table, *l3_table;

    pte ptte = k_code_table_mapping;
    pte ptme = k_code_mem_mapping;

    // mark permissions for the mapped regions. See `mmu_types.h` for AccessPermission values
    switch(perms){
        case RO:
            ptme.md.ap = 2;
            ptme.md.pxn = 1;
            break;
        case RW:
            ptme.md.ap = 0;
            ptme.md.pxn = 1;
            break;
        case RX:
            ptme.md.ap = 2;
            ptme.md.pxn = 0;
            break;
    }

    for(int i = 0; i < l3_blocks; i++){
        idx0 = (va >> 39) & 0x1FF;
        idx1 = (va >> 30) & 0x1FF;
        idx2 = (va >> 21) & 0x1FF;
        idx3 = (va >> 12) & 0x1FF;

        if(!l0_table[idx0].td.valid){
            ptte.td.address = (alloc_page_table_lo()) >> PAGE_SHIFT;
            l0_table[idx0] = ptte;
        }
        l1_table = (pte*) ((u64) l0_table[idx0].td.address << PAGE_SHIFT);
        if (!l1_table[idx1].td.valid) {
            ptte.td.address = (alloc_page_table_lo()) >> PAGE_SHIFT;
            l1_table[idx1] = ptte;
        }

        l2_table = (pte*) ((u64) l1_table[idx1].td.address << PAGE_SHIFT);
        if (!l2_table[idx2].td.valid) {
            ptte.td.address = (alloc_page_table_lo()) >> PAGE_SHIFT;
            l2_table[idx2] = ptte;
        }

        l3_table = (pte*) ((u64) l2_table[idx2].td.address << PAGE_SHIFT);
        if (!l3_table[idx3].md.valid) {
            ptme.md.address = (pa >> PAGE_SHIFT);
            l3_table[idx3] = ptme;
        }

        va += PAGE_SIZE;
        pa += PAGE_SIZE;
    }
}

static BOOT_FN void map_l2_range(u64 va, u64 pa, int l2_blocks, u64* pt, enum permissions perms){
    u32 idx0, idx1, idx2;

    pte* l0_table = (pte*) pt;
    pte* l1_table;
    pte* l2_table;

    pte ptte = k_code_table_mapping;
    pte ptme = k_code_mem_mapping;

    // mark type as 0 since l2 memory blocks need this (l3 pages have a type of 1)
    ptme.md.type = 0;

    // mark permissions for the mapped regions. See `mmu_types.h` for AccessPermission values
    switch(perms){
        case RO:
            ptme.md.ap = 2;
            ptme.md.pxn = 1;
            break;
        case RW:
            ptme.md.ap = 0;
            ptme.md.pxn = 1;
            break;
        case RX:
            ptme.md.ap = 2;
            ptme.md.pxn = 0;
            break;
    }

    for(int i = 0; i < l2_blocks; i++){
        idx0 = (va >> 39) & 0x1FF;
        idx1 = (va >> 30) & 0x1FF;
        idx2 = (va >> 21) & 0x1FF;

        if(!l0_table[idx0].td.valid){
            ptte.td.address = (alloc_page_table_lo()) >> PAGE_SHIFT;
            l0_table[idx0] = ptte;
        }
        l1_table = (pte*) ((u64) l0_table[idx0].td.address << PAGE_SHIFT);
        if (!l1_table[idx1].td.valid) {
            ptte.td.address = (alloc_page_table_lo()) >> PAGE_SHIFT;
            l1_table[idx1] = ptte;
        }

        l2_table = (pte*) ((u64) l1_table[idx1].td.address << PAGE_SHIFT);
        if (!l2_table[idx2].md.valid) {
            ptme.md.address = (pa >> PAGE_SHIFT);
            l2_table[idx2] = ptme;
        }

        va += (PAGE_SIZE << 9);
        pa += (PAGE_SIZE << 9);
    }
}

static BOOT_FN void map_lo(u64 va, u64 pa, int pages, u64* pt, enum permissions perms){
    // get number of pages va is from an offset of 2MiB
    u64 l2_boundary = (ALIGN_UP(va, (PAGE_SIZE << 9)) - va);
    u64 n_front_pages = l2_boundary / PAGE_SIZE;
    if(n_front_pages){
        n_front_pages = MIN(n_front_pages, pages);
        map_l3_range(va, pa, n_front_pages, pt, perms);
        va += n_front_pages * PAGE_SIZE;
        pa += n_front_pages * PAGE_SIZE;
        pages -= n_front_pages;
    }

    u64 l2_blocks = pages >> 9;     // pages / 512
    u64 l3_blocks = pages & 0x1FF;  // pages % 512

    if(l2_blocks){
        map_l2_range(va, pa, l2_blocks, pt, perms);
        va += l2_blocks * (PAGE_SIZE << 9);
        pa += l2_blocks * (PAGE_SIZE << 9);
    }

    if(l3_blocks) map_l3_range(va, pa, l3_blocks, pt, perms);
}
// ––––––––––––––––––––––––––––––––––––––––––––––– //

// ––––––––––––––– MAPPING FUNCTIONS ––––––––––––––– //
static BOOT_FN void map_pre_code(u64* pt){
    // align the kernel start down to the nearest page boundary
    u64 code_base = ALIGN_DOWN(SYM_ABS(__kernel_high_start), PAGE_SIZE);
    u64 vb = VIRT_BASE;
    
    u64 pages_to_map = (code_base - vb) / PAGE_SIZE;

    map_lo(vb, vb - vb, pages_to_map, pt, RW);
}

static BOOT_FN void map_code(u64* pt){
    // align the kernel start down to the nearest page boundary
    u64 code_base = ALIGN_DOWN(SYM_ABS(__text_start), PAGE_SIZE);
    u64 code_end = ALIGN_UP(SYM_ABS(__text_end), PAGE_SIZE);
    u64 vb = VIRT_BASE;
    
    u64 pages_to_map = (code_end - code_base) / PAGE_SIZE;

    map_lo(code_base, code_base - vb, pages_to_map, pt, RX);
}

static BOOT_FN void map_rodata(u64* pt){
    u64 rodata_start = ALIGN_DOWN(SYM_ABS(__rodata_start), PAGE_SIZE);
    u64 rodata_end = ALIGN_UP(SYM_ABS(__rodata_end), PAGE_SIZE);
    u64 vb = VIRT_BASE;

    u64 pages_to_map = (rodata_end - rodata_start) / PAGE_SIZE;

    map_lo(rodata_start, rodata_start - vb, pages_to_map, pt, RO);
}

static BOOT_FN void map_bss(u64* pt){
    u64 bss_start = ALIGN_DOWN(SYM_ABS(__bss_start), PAGE_SIZE);
    u64 bss_end = ALIGN_UP(SYM_ABS(__bss_end), PAGE_SIZE);
    u64 vb = virt_base_lo();

    u64 pages_to_map = (bss_end - bss_start) / PAGE_SIZE;

    map_lo(bss_start, bss_start - vb, pages_to_map, pt, RW);
}

static BOOT_FN void map_static_page_region(u64* pt){
    u64 vb = VIRT_BASE;
    u64 spr = SYM_ABS(__static_page_region_start);

    u64 pages_to_map = static_page_region_pages();

    map_lo(spr, spr - vb, pages_to_map, pt, RW);
}

static BOOT_FN void map_kernel_stack(u64* pt){
    // bottom < top
    u64 kstack_bottom = ALIGN_DOWN(SYM_ABS(__kstack_bottom_hi), PAGE_SIZE);
    u64 kstack_top = ALIGN_UP(SYM_ABS(__kstack_top_hi), PAGE_SIZE);
    u64 vb = VIRT_BASE;

    u64 pages_to_map = (kstack_top - kstack_bottom) / PAGE_SIZE;

    map_lo(kstack_bottom, kstack_bottom - vb, pages_to_map, pt, RW);
}

static BOOT_FN void map_mmu_stub(u64* pt){
    u64 mmu_stub_loc = (u64) &enable_mmu;
    mmu_stub_loc = ALIGN_DOWN(mmu_stub_loc, PAGE_SIZE);

    // map single page into lower memory for the stub
    map_lo(mmu_stub_loc, mmu_stub_loc, 1, pt, RX);
}

static BOOT_FN void map_page_frame_array(u64* pt){
    u64 start_addr = ALIGN_DOWN(SYM_ABS(__page_frame_array_start), PAGE_SIZE);
    u64 end_addr = ALIGN_UP(SYM_ABS(__page_frame_array_end), PAGE_SIZE);
    u64 vb = VIRT_BASE;

    u64 pages_to_map = (end_addr - start_addr) >> 12;

    map_lo(start_addr, start_addr - vb, pages_to_map, pt, RW);
}


static BOOT_FN void map_ram(u64* pt){
    // work around the existing spots to fill all of RAM. Only populates first GIB

    u64 test_section_start = (u64) __test_phys_loc;
    u64 test_size = (u64) __test_size;
    u64 test_end = ALIGN_UP(test_section_start + test_size, PAGE_SIZE);
    u64 vb = VIRT_BASE;

    // 1 GiB minus the end of the test section
    u64 pages_to_map = ((1 << 30) - test_end) / PAGE_SIZE;

    // mark the remainder of the first GB as read-write for the kernel
    map_lo(vb + test_end, test_end, pages_to_map, pt, RW);
}
// ––––––––––––––––––––––––––––––––––––––––––––––––– //



/**
 * @brief Sets up the memory mapping for the kernel to run in high-half land. 
 * This ONLY pertains to initializing the code and stack segments for the kernel.
 * The rest of initialization will happen in high-half.
 */
void BOOT_FN map_high(){
    k_code_table_mapping.td.type = 1;
    k_code_table_mapping.td.pxn = 1;
    k_code_table_mapping.td.uxn = 1;
    k_code_table_mapping.td.valid = 1;

    k_code_mem_mapping.md.valid = 1;
    k_code_mem_mapping.md.af = 1;
    k_code_mem_mapping.md.ns = 0;
    k_code_mem_mapping.md.sh = 3;
    k_code_mem_mapping.md.ap = 0;
    k_code_mem_mapping.md.pxn = 1;
    k_code_mem_mapping.md.uxn = 1;
    k_code_mem_mapping.md.attr_index = 0;
    k_code_mem_mapping.md.type = 1;

    u64* pt_hi = (u64*) alloc_page_table_lo();
    u64* pt_lo = (u64*) alloc_page_table_lo();

    // 1. Mark early pre-kernel code address range as R/W
    map_pre_code(pt_hi);

    // 2. Mark kernel code as R/X
    map_code(pt_hi);

    // 3. Mark kernel rodata as RO
    map_rodata(pt_hi);

    // 4. Mark kernel BSS as RW
    map_bss(pt_hi);

    // 5. Mark static page region for MMU initialization
    map_static_page_region(pt_hi);

    // 6. Map kstack
    map_kernel_stack(pt_hi);

    // 7. Complete the rest of the RAM mapping
    map_mmu_stub(pt_lo);

    // 8. Map the page frame array, so generic map can use buddy allocator
    map_page_frame_array(pt_hi);

    // 9. Complete the rest of the RW RAM mapping
    map_ram(pt_hi);

    // 10. Enable MMU and complete kernel boot
    enable_mmu(pt_hi, pt_lo);
}