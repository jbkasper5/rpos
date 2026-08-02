#include "memory/memprofiler.h"
#include "memory/paging.h"
#include "types/paging_types.h"
#include "types/process_types.h"
#include "types/mmu_types.h"
#include "asm_utils.h"
#include "memory/virtual_memory.h"

extern list_head_t proclist;


static void walk_virtual_memory(pte* parent_table, u8 level){
    pte* child_table = NULL;
    u32 n_entries = PAGE_SIZE / 8;
    page_frame_t* base = page_frame_array_start();
    for(int i = 0; i < n_entries; i++){
        if(!parent_table[i].md.valid) continue;

        if(level < 3 && parent_table[i].td.type == 1) {
            // This is a table, recurse!
            walk_virtual_memory(pa_to_va(parent_table[i].td.address << 12), level + 1);
        }else{
            u64 addr = parent_table[i].md.address << 12;
            page_frame_t* curr = base + parent_table[i].md.address;
            kprintf("\tlv %d: Leaf node address: 0x%x\n", level, addr);
            if(curr->flags.bits.state == PAGE_RESERVED){
                kprintf("\t\tAddr 0x%x: [RESERVED]\n", addr);
            }else{
                kprintf("\t\tAddr 0x%x: [.ref = %d, .order = 0x%x, .state = 0x%x, .flags = 0x%x]\n", addr, curr->refcount, curr->order, curr->flags.bits.state, curr->flags.bits.flags);
            }
        }
    }
    return;
}
static void profile_pid(u64 pid){
    if(list_empty(&proclist)){
        ERROR("Could not find pid '%d' to profile.\n", pid);
    }

    pcb_t* proc = list_entry(proclist.next, pcb_t, proclist);
    bool found = FALSE;

    do{
        if(proc->pid == pid){
            found = TRUE;
            break;
        }else{
            proc = list_entry(proc->proclist.next, pcb_t, proclist);
        }
    }while(proc != proclist.prev);

    if(!found){
        ERROR("Could not find pid '%d' to profile.\n", pid);
    }else{
        kprintf("Profiling memory starting from L0 '0x%x'\n", proc->ttbr);
        walk_virtual_memory(pa_to_va(proc->ttbr), 0);
    }
}




void profile(memprofiler_cfg* cfg){
    if(cfg->pid) profile_pid(cfg->pid);

    page_frame_t* base = page_frame_array_start();
    page_frame_t* base_end = page_frame_array_end();
    page_frame_t* start = base + cfg->start_pfn;
    u64 n_pages = (cfg->count) ? cfg->count : base_end - start;
    kprintf("Profiling %d pages...\n", n_pages);

    u64 start_addr = (start - base) << 12;
    page_frame_t* curr = start;
    // for(int i = 0; i < n_pages; i++){
    //     kprintf("   Addr 0x%x: [.order = 0x%x, .state = 0x%x, .flags = 0x%x]\n", start_addr, curr->order, curr->flags.bits.state, curr->flags.bits.flags);
    //     start_addr += (1 << 12);
    //     curr++;
    // }
}

void ram_profile(){

}
