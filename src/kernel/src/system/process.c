#include "system/process.h"
#include "memory/mmap.h"
#include "filedescriptors/filedescriptors.h"
#include "system/entry.h"

                       //0x00007fffffff0
#define USER_STACK_TOP   0x0000800000000ULL
#define KSTACK_SIZE      PAGE_SIZE

static u32 pidcounter = 1;
extern void ret_from_fork();

static void* initialize_proc_kstack(){
    u64 range = buddy_alloc(KSTACK_SIZE + PAGE_SIZE);
    map(range + PAGE_SIZE, va_to_pa(range + PAGE_SIZE), 0, MAP_KERNEL | MAP_READ | MAP_WRITE, L0_TABLE);   

    return range + (KSTACK_SIZE + PAGE_SIZE) - 0x10;
}

u32 generate_pid(){
    return atomic_increment(&pidcounter, 1);
}

int fd_alloc(pcb_t* proc, file_t* file) {
    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        if(!proc->fds[i]) {
            proc->fds[i] = file;
            return i;
        }
    }
    return -1;
    // return -EMFILE;  // too many open files
}

pcb_t* procalloc(u64 entrypoint){
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

    void* kstack = initialize_proc_kstack();
    process->kernel_stack = kstack;

    // set the kernel stack to the trap frame defined by the kernel_entry of the parent process
    process->kernel_stack -= S_FRAME_SIZE;

    // in order to be context switched in later, we need to do an "artificial push" of the x19-x30 registers
    // in total, there are 12 registers to "push", each 8 bytes, so 96 bytes of context to invent
    process->kernel_stack -= 96;
    memset(process->kernel_stack, 0, 96 + S_FRAME_SIZE);

    // set x30 from the context switch to be the trampoline out of the kernel
    *((u64*) (process->kernel_stack + 8)) = &ret_from_fork;

    u64* trap_frame = (u64*)(process->kernel_stack + 96);
    trap_frame[31] = USER_STACK_TOP - 16;               // SP_EL0
    trap_frame[32] = entrypoint;                        // ELR_EL1
    trap_frame[33] = 0x0;                               // SPSR


    // subtract 16 since USER_STACK_TOP technically lies outside the 2 page boundary
    process->registers.sp = USER_STACK_TOP - 16;

    file_t* stdin_f  = kmalloc(sizeof(file_t));
    file_t* stdout_f = kmalloc(sizeof(file_t));
    file_t* stderr_f = kmalloc(sizeof(file_t));

    // struct copy
    *stdin_f  = stdio_file;
    *stdout_f = stdio_file;
    *stderr_f = stdio_file;

    // set the defaults for all processes
    process->fds[STDIN] = stdin_f;
    process->fds[STDOUT] = stdout_f;
    process->fds[STDERR] = stderr_f;

    return process;
}

pcb_t* clone_active_proc(){
    pcb_t* process = (pcb_t*) kmalloc(sizeof(pcb_t));

    // copy parent process data into child process
    memcpy(process, get_current(), sizeof(pcb_t));

    process->kernel_stack = initialize_proc_kstack();

    u64 current_sp;
    asm volatile("mov %0, sp" : "=r"(current_sp));

    u64 head = head_from_page(ALIGN_DOWN(current_sp, PAGE_SIZE)); 

    u64 current_stack_top = head + PAGE_SIZE;

    // copy the kernel stack
    memcpy(ALIGN_DOWN(process->kernel_stack, KSTACK_SIZE),  current_stack_top, PAGE_SIZE);

    // now we need to create a new virtual memory system (using the virt address of the page table)
    process->registers.ttbr = clone_virtual_memory(pa_to_va(get_current()->registers.ttbr));

    // set the kernel stack to the trap frame defined by the kernel_entry of the parent process
    process->kernel_stack -= S_FRAME_SIZE;

    // set process's return value to 0, for child PID
    *((u64*) (process->kernel_stack)) = 0;

    // in order to be context switched in later, we need to do an "artificial push" of the x19-x30 registers
    // in total, there are 12 registers to "push", each 8 bytes, so 96 bytes of context to invent
    process->kernel_stack -= 96;
    memset(process->kernel_stack, 0, 96);

    u64* trap_frame = (u64*)(process->kernel_stack + 96);

    // set the x30 return address to be the trampoline out of the kernel
    *((u64*) (process->kernel_stack + 8)) = &ret_from_fork;

    // return copied child process
    return process;
}