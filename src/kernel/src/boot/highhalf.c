#include "macros.h"
#include "memory/mmap.h"

extern uint32_t static_page_region_pages();
extern uintptr_t static_page_region_start();
extern uintptr_t static_page_region_end();

extern uint64_t virt_base();
extern uint64_t kernel_high_start();
extern uint64_t kernel_high_end();
extern uint64_t kernel_end();

extern void enable_mmu();

static BOOT_BSS table_descriptor_t k_code_table_mapping = {0};
static BOOT_BSS mem_descriptor_t k_code_mem_mapping = {0};
static BOOT_BSS int lo_allocated_pages = 0;


static BOOT_FN uint64_t alloc_page_table_lo(){
    // get physical address of the allocated pages variable
    // int* var = UNSCALED_POINTER_SUB(&allocated_pages, virt_base());
    uint64_t* page_addr = UNSCALED_POINTER_ADD(static_page_region_start_phys(), (PAGE_SIZE * lo_allocated_pages));
    lo_allocated_pages++;
    for(int i = 0; i < PAGE_SIZE / sizeof(uint64_t); i++) page_addr[i] = 0;
    return (uint64_t*) page_addr;
}

static BOOT_FN void map_lo(uint64_t va, uint64_t pa, int pages, uint64_t* pt){
    uint32_t idx0, idx1, idx2, idx3;

    table_descriptor_t* l0_table = (table_descriptor_t*) pt;
    table_descriptor_t* l1_table;
    table_descriptor_t* l2_table;
    mem_descriptor_t* l3_table;

    table_descriptor_t ptte = k_code_table_mapping;
    mem_descriptor_t ptme = k_code_mem_mapping;

    for(int i = 0; i < pages; i++){
        idx0 = (va >> 39) & 0x1FF;
        idx1 = (va >> 30) & 0x1FF;
        idx2 = (va >> 21) & 0x1FF;
        idx3 = (va >> 12) & 0x1FF;

        if(!l0_table[idx0].bits.valid){
            ptte.bits.address = (alloc_page_table_lo()) >> PAGE_SHIFT;
            l0_table[idx0] = ptte;
        }
        l1_table = (table_descriptor_t*) ((uint64_t) l0_table[idx0].bits.address << PAGE_SHIFT);
        if (!l1_table[idx1].bits.valid) {
            ptte.bits.address = (alloc_page_table_lo()) >> PAGE_SHIFT;
            l1_table[idx1] = ptte;
        }

        l2_table = (table_descriptor_t*) ((uint64_t) l1_table[idx1].bits.address << PAGE_SHIFT);
        if (!l2_table[idx2].bits.valid) {
            ptte.bits.address = (alloc_page_table_lo()) >> PAGE_SHIFT;
            l2_table[idx2] = ptte;
        }

        l3_table = (mem_descriptor_t*) ((uint64_t) l2_table[idx2].bits.address << PAGE_SHIFT);
        if (!l3_table[idx3].bits.valid) {
            ptme.bits.address = (pa >> PAGE_SHIFT);
            l3_table[idx3] = ptme;
        }

        va += PAGE_SIZE;
        pa += PAGE_SIZE;
    }
}

static BOOT_FN void map_static_page_region(uint64_t* pt){
    uint64_t vb = virt_base();
    uint64_t spr = static_page_region_start();

    uint64_t pages_to_map = static_page_region_pages();

    map_lo(spr + vb, spr, pages_to_map, pt);
}

static BOOT_FN void identity_map_code(uint64_t* pt){
    uint64_t code_base = ALIGN_DOWN(kernel_high_start(), PAGE_SIZE);
    uint64_t code_end = ALIGN_UP(kernel_high_end(), PAGE_SIZE);
    uint64_t vb = virt_base();
    
    uint64_t reserved_memory = (uint64_t) static_page_region_end();
    uint64_t reserved_pages = (reserved_memory + 0xFFF) >> 12;

    map_lo(PAGE_SIZE, PAGE_SIZE, reserved_pages, pt);
}

static BOOT_FN void map_code(uint64_t* pt){
    // align the kernel start down to the nearest page boundary
    uint64_t code_base = ALIGN_DOWN(kernel_high_start(), PAGE_SIZE);
    uint64_t code_end = ALIGN_UP(kernel_high_end(), PAGE_SIZE);
    uint64_t vb = virt_base();
    
    uint64_t pages_to_map = (code_end - code_base) / PAGE_SIZE;

    map_lo(code_base, code_base - vb, pages_to_map, pt);
}

/**
 * @brief Sets up the memory mapping for the kernel to run in high-half land. 
 * This ONLY pertains to initializing the code and stack segments for the kernel.
 * The rest of initialization will happen in high-half.
 */
void BOOT_FN map_high(){

    k_code_table_mapping.bits.type = 1;
    k_code_table_mapping.bits.pxn = 0;
    k_code_table_mapping.bits.uxn = 1;
    k_code_table_mapping.bits.valid = 1;

    k_code_mem_mapping.bits.valid = 1;
    k_code_mem_mapping.bits.af = 1;
    k_code_mem_mapping.bits.ns = 0;
    k_code_mem_mapping.bits.sh = 3;
    k_code_mem_mapping.bits.ap = 0;
    k_code_mem_mapping.bits.pxn = 0;
    k_code_mem_mapping.bits.uxn = 1;
    k_code_mem_mapping.bits.attr_index = 0;
    k_code_mem_mapping.bits.type = 1;


    uint64_t* pt_hi = (uint64_t*) alloc_page_table_lo();
    uint64_t* pt_lo = (uint64_t*) alloc_page_table_lo();

    map_static_page_region(pt_hi);

    map_code(pt_hi);

    uint32_t hi_pages_used = lo_allocated_pages;

    identity_map_code(pt_lo);

    enable_mmu(pt_hi, pt_lo);
}


/*
0x80F = 1000 0000 1111
= ,1 | 0 | 0 | 0 | ,0 | 000 ,111 | 1

^ Level 3 address translation fault
*/