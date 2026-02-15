#include "system/process.h"
#include "memory/mmap.h"

#define USER_STACK_TOP   0x0000800000000000ULL   // 2^47

pcb_t* procalloc(){
    // allocate new process control block
    pcb_t* process = (pcb_t*) kmalloc(sizeof(pcb_t));

    memset(process, 0, sizeof(pcb_t));

    // allocate and map the L0 page table for the process
    // the kernel should be able to dereference this table, but not the process
    process->registers.ttbr = alloc_page_table();

    // 8 KiB stack
    uint32_t stack_size = 1 << 13;
    uint64_t stack_base = buddy_alloc(1 << 13);

    // map the virtual stack to the process
    map(USER_STACK_TOP, stack_base, 2, MAP_USER | MAP_READ | MAP_WRITE, process->registers.ttbr);
    process->registers.sp = USER_STACK_TOP;
    
    return process;
}

pcb_t* initialize_stack(pcb_t* proc){

}