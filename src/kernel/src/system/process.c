#include "system/process.h"
#include "memory/mmap.h"
#include "filedescriptors/filedescriptors.h"

#define USER_STACK_TOP   0x0000800000000ULL

const fileops_t stdout_ops = {
    .write = uart_write,
    .read = uart_read,
    .close = NULL,
    .ioctl = NULL,
};

pcb_t* procalloc(){
    // allocate new process control block
    pcb_t* process = (pcb_t*) kmalloc(sizeof(pcb_t));

    memset(process, 0, sizeof(pcb_t));

    // allocate and map the L0 page table for the process
    // the kernel should be able to dereference this table, but not the process
    process->registers.ttbr = alloc_page_table();

    // 8 KiB stack -> 2 pages -> order 1
    u32 stack_size = PAGE_SIZE * 2;
    u64 stack_base = buddy_alloc(stack_size); // stack base page address = 3FFD5000

    // map the virtual stack to the process (0x7ffffe000 -> 3FFD5000)
    map(USER_STACK_TOP - stack_size, va_to_pa(stack_base), 1, MAP_USER | MAP_READ | MAP_WRITE, process->registers.ttbr);

    // subtract 16 since USER_STACK_TOP technically lies outside the 2 page boundary
    process->registers.sp = USER_STACK_TOP - 16;

    process->fds[STDIN].file_ops = &stdout_ops;
    process->fds[STDOUT].file_ops = &stdout_ops;
    process->fds[STDERR].file_ops = &stdout_ops;
    
    return process;
}

pcb_t* clone_active_proc(){
    pcb_t* process = (pcb_t*) kmalloc(sizeof(pcb_t));

    // copy parent process data into child process
    memcpy(process, &proclist.proclist[active_process], sizeof(pcb_t));

    // return copied child process
    return process;
}

pcb_t* initialize_stack(pcb_t* proc){

}