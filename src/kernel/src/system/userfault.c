#include "macros.h"
#include "system/process.h"
#include "memory/mem.h"
#include "memory/paging.h"
#include "memory/mmap.h"

void handle_el0_fault();


static void halt(){
    u64 esr = READ_SYSREG(esr_el1);
    u64 elr = READ_SYSREG(elr_el1);
    u64 far = READ_SYSREG(far_el1);

    ERROR("\tException: 0x%x\n", esr);
    ERROR("\tFaulting instruction: 0x%x\n", elr);
    ERROR("\tAddress causing fault: 0x%x\n", far);

    while(TRUE);
}

static bool check_vma(u64 faulting_address, u64 exception_status){
    pcb_t* current = get_current();
    for (list_head_t* p = current->vmas.next; p != &current->vmas; p = p->next){
        vma* v = list_entry(p, vma, list);
        // process v
        // check if the faulting address is contained within the 
        if(faulting_address < v->end && faulting_address >= v->start){
            // now check if the permissions match
            if(TRUE){
                // allocate a page for the fault memory for it
                u64* phys = va_to_pa(buddy_alloc(PAGE_SIZE));

                // map the memory to the process, marking it as a user section
                map(ALIGN_DOWN(faulting_address, PAGE_SIZE), phys, 0, MAP_USER | v->permissions, current->ttbr);

                // resolved the faulting address with lazy mapping, so we can return to user space
                return TRUE;
            }
        }
    }

    // faulting address was not contained within a registered VMA, so it's a genuine fault
    return FALSE;
}

pte* check_cow(u64 addr){
    pcb_t* current = get_current();
    pte* page_table = (pte*) current->ttbr;

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
        DEBUG("L2 entry: type=%d (1=table, 3=block) value=0x%x\n", entry.td.type, entry.value);
        page_table = (pte*) pa_to_va(entry.td.address << 12);
    }else{
        DEBUG("L2 translation failed.\n");
        return NULL;
    }

    entry = page_table[idx3];
    if(entry.md.cow){
        DEBUG("COW recognized at L3[%d], PTE=0x%x\n", idx3, entry.value);
        return page_table + idx3;
    }else{
        DEBUG("Page not marked for COW.\n");
    }

    return NULL;
}

void handle_el0_fault(u64 faulting_address, u64 exception_status){
    DEBUG("EL0 fault @ 0x%x (idx0=%d idx1=%d idx2=%d idx3=%d)\n",
      faulting_address,
      (faulting_address >> 39) & 0x1FF,
      (faulting_address >> 30) & 0x1FF,
      (faulting_address >> 21) & 0x1FF,
      (faulting_address >> 12) & 0x1FF);

    pte* l3_pte = check_cow(faulting_address);
    void* current_ttbr = get_current()->ttbr;

    // if we lie in the VMA and 
    if(l3_pte){
        // copy the memory to a new table
        void* new_page = buddy_alloc(PAGE_SIZE);
        void* base_addr = ALIGN_DOWN(faulting_address, PAGE_SIZE);

        // map the page in the kernel so we can copy it
        map(new_page, va_to_pa(new_page), 0, MAP_KERNEL | MAP_READ | MAP_WRITE, L0_TABLE);

        // now that the new address is properly mapped, we can copy it
        memcpy(new_page, base_addr, PAGE_SIZE);

        // get the pfn of the page we need to update
        u64 pfn = (u64) l3_pte->md.address;
        page_frame_t* start = pa_to_va(page_frame_array_start());
        page_frame_t* pf = start + pfn;


        // if the refcount has dropped far enough, then we don't need to designate it as COW anymore
        if(pf->refcount == 1){
            l3_pte->md.cow = 0;

            // mark it as read/write again
            // BUG: should restore permission set before it was marked RO
            l3_pte->md.ap = EL0_RW_EL1_RW;
        }

        // decrement the refcounter for the page we split a copy from
        decrement_ref(pa_to_va(pfn << PAGE_SHIFT));

        // update the PTE address to point to the new page
        l3_pte->md.address = va_to_pa((u64) new_page) >> 12;

        // flush TLB now that we've updated virtual memory 
        flush_tlb();
    }else{
        // if the faulting address is not within a VMA, halt for genuine crash
        if(!check_vma(faulting_address, exception_status)) halt();
    }
}


/*
Cases:
    case 1: Normal operation, single parent single child
        - next person to access that page clones it, decrements ref, marks cow pending as 0 and makes it R/W again
            - this means the next access to this page won't fault, whoever remains to use it

*/