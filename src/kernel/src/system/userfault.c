#include "macros.h"
#include "system/process.h"
#include "memory/mem.h"
#include "memory/paging.h"
#include "memory/mmap.h"

void handle_el0_fault();

pte* check_cow(u64 addr){
    pcb_t* current = get_current();
    pte* page_table = (pte*) pa_to_va(current->registers.ttbr);

    int idx0 = (addr >> 39) & 0x1FF;
    int idx1 = (addr >> 30) & 0x1FF;
    int idx2 = (addr >> 21) & 0x1FF;
    int idx3 = (addr >> 12) & 0x1FF;
    int page_off = addr & 0xFFF;

    pte entry = page_table[idx0];
    if(entry.td.valid){
        page_table = (pte*) pa_to_va(entry.td.address << 12);
    }else{
        DEBUG("L0 translation failed.\n");
        return NULL;
    }

    entry = page_table[idx1];
    if(entry.td.valid){
        page_table = (pte*) pa_to_va(entry.td.address << 12);
    }else{
        DEBUG("L1 translation failed.\n");
        return NULL;
    }

    entry = page_table[idx2];
    if(entry.td.valid){
        page_table = (pte*) pa_to_va(entry.td.address << 12);
    }else{
        DEBUG("L2 translation failed.\n");
        return NULL;
    }

    entry = page_table[idx3];
    if(entry.md.cow){
        DEBUG("COW Pending recognized.\n");
        return page_table + idx3;
    }else{
        DEBUG("Page not marked for COW.\n");
    }

    return NULL;
}

void handle_el0_fault(u64 faulting_address){
    DEBUG("EL0 Exception detected.\n");
    pte* l3_pte = check_cow(faulting_address);
    void* current_ttbr = get_current()->registers.ttbr;

    if(l3_pte){
        // copy the memory to a new table
        void* new_page = buddy_alloc(PAGE_SIZE);
        void* base_addr = ALIGN_DOWN(faulting_address, PAGE_SIZE);

        // map the page in the kernel so we can copy it
        map(new_page, va_to_pa(new_page), 0, MAP_KERNEL | MAP_READ | MAP_WRITE, L0_TABLE);

        // now that the new address is properly mapped, we can copy it
        memcpy(new_page, base_addr, PAGE_SIZE);

        // mark the page table cow as 0 now that we've addressed it
        l3_pte->md.cow = 0;

        // mark it as read/write again
        l3_pte->md.ap = EL0_RW_EL1_RW;

        // update the PTE address to point to the new page
        l3_pte->md.address = va_to_pa((u64) new_page) >> 12;

        flush_tlb();
    }else{
        // do nothing ig
    }
}