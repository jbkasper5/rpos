#include "system/process.h"
#include "memory/mmap.h"
#include "filedescriptors/filedescriptors.h"
#include "system/entry.h"
#include "types/kernel_types.h"
#include "uabi/rpos/errno.h"

#define USER_STACK_TOP   0x0000800000000ULL
#define KSTACK_SIZE      PAGE_SIZE

static u32 pidcounter = 0;
extern void ret_from_fork();

static void* initialize_proc_kstack(){
    u64 range = buddy_alloc(KSTACK_SIZE + PAGE_SIZE);
    if(!range) return NULL;

    u64 kstack = (range + (KSTACK_SIZE + PAGE_SIZE) - 0x10);

    // set the kernel stack to the trap frame defined by the kernel_entry of the parent process
    trap_frame_t* tf = BASE_TRAPFRAME(kstack);
    memset(tf, 0, sizeof(trap_frame_t));

    // in order to be context switched in later, we need to do an "artificial push" of the x19-x30 registers
    // in total, there are 12 registers to "push", each 8 bytes, so 96 bytes of context to invent
    kstack = kstack - sizeof(trap_frame_t) - 96;
    memset((void*) kstack, 0, 96);

    // set x30 from the context switch to be the trampoline out of the kernel
    *((u64*) (kstack + 8)) = (u64) &ret_from_fork;

    return (void*) kstack;
}

static void set_state(pcb_t* pcb, enum PROC_STATE state){
    pcb->state = state;
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
    return -EMFILE;  // too many open files
}

void procreap(pcb_t* proc){
    // free cwd buffer

    // release the pcb
    kfree(proc);

    return TRUE;
}

pcb_t* pcb_allocate(){
    // allocate new process control block
    pcb_t* process = (pcb_t*) kmalloc(sizeof(pcb_t));
    if(!process){
        ERROR("No memory left to allocate pcb.\n");
        return NULL;
    }
    memset(process, 0, sizeof(pcb_t));

    process->cwd = (char*) kmalloc(MAX_PATH);
    if(!process->cwd){
        ERROR("No memory left to allocate cwd buffer.\n");
        kfree(process);
        return NULL;
    }

    process->pid = generate_pid();

    // initialize process bookkeeping lists
    INIT_LIST_HEAD(&process->children);
    INIT_LIST_HEAD(&process->siblings);
    INIT_LIST_HEAD(&process->proclist);
    INIT_LIST_HEAD(&process->runqueue);

    // initialize the VMA list
    INIT_LIST_HEAD(&process->vmas);

    // not waiting on anyone by default
    process->waiting_on = WAITING_NONE;

    // set up the kernel stack with the trampoline out of EL1
    process->kernel_stack = (u64) initialize_proc_kstack();
    if(!process->kernel_stack){
        ERROR("No memory left to allocate kernel stack.\n");
        kfree(process->cwd);
        kfree(process);
        return NULL;
    }

    // mark the process as uninitialized
    set_state(process, PROCESS_UNINITIALIZED);

    return process;
}

static int initialize_from_pcb(pcb_t* proc, pcb_t* cloneproc){
    proc->ttbr = (u64) clone_virtual_memory((pte*) cloneproc->ttbr);
    if(!proc->ttbr){
        ERROR("Could not copy virtual memory.\n");
        return -ENOMEM;
    }

    proc->cwd = (char*) kmalloc(MAX_PATH);
    if(!proc->cwd){
        ERROR("Could not allocate cwd buffer.\n");
        return -ENOMEM;
    }

    trap_frame_t* clone_tf = BASE_TRAPFRAME(cloneproc->kernel_stack);
    trap_frame_t* proc_tf = BASE_TRAPFRAME(proc->kernel_stack);

    // copy the trapframe, then set the $x0 value to 0 to denote the child PID
    memcpy(proc_tf, clone_tf, sizeof(trap_frame_t));
    proc_tf->regs[0] = 0;


    // copy all the files over
    memcpy(proc->fds, cloneproc->fds, sizeof(file_t*) * MAX_OPEN_FILES);

    // since we're copying pointers to the files, we need to increment their refs
    for(int i = 0; i < MAX_OPEN_FILES; i++){
        if(proc->fds[i]) proc->fds[i]->refcount++;
    }

    // iterate over parent VMAs and copy then into the new process
    for (list_head_t* p = cloneproc->vmas.prev; p != &cloneproc->vmas; p = p->prev){

        // copy parent process VMA into new child VMA list
        vma* v = list_entry(p, vma, list);
        vma* new_vma = (vma*) kmalloc(sizeof(vma));
        if(!new_vma) panic();
        memcpy(new_vma, v, sizeof(vma));
        list_add(&new_vma->list, &proc->vmas);
    }

    proc->start_brk = cloneproc->start_brk;
    proc->brk = cloneproc->brk;

    // set the parent
    proc->parent = cloneproc;
}

static int initialize_fresh(pcb_t* proc){
    file_t* stdin_f  = kmalloc(sizeof(file_t));
    if(!stdin_f){
        ERROR("Could not allocate stdin file.\n");
        return -ENOMEM;
    }
    file_t* stdout_f = kmalloc(sizeof(file_t));
    if(!stdout_f){
        ERROR("Could not allocate stdout file.\n");
        return -ENOMEM;
    }
    file_t* stderr_f = kmalloc(sizeof(file_t));
    if(!stderr_f){
        ERROR("Could not allocate stderr file.\n");
        return -ENOMEM;
    }

    // struct copy
    *stdin_f  = stdio_file;
    *stdout_f = stdio_file;
    *stderr_f = stdio_file;

    // set the defaults for all processes
    proc->fds[STDIN] = stdin_f;
    proc->fds[STDOUT] = stdout_f;
    proc->fds[STDERR] = stderr_f;

    proc->parent = get_current();
    proc->ttbr = buddy_alloc_pt();
    if(!proc->ttbr){
        ERROR("Could not allocate fresh TTBR0\n");
        return -ENOMEM;
    }

    return TRUE;
}

void pcb_init(pcb_t* pcb, pcb_t* cloneproc){
    if(cloneproc) initialize_from_pcb(pcb, cloneproc);
    else initialize_fresh(pcb);

    // tf->sp_el0 = USER_STACK_TOP - 16;
    // tf->elr_el1 = entrypoint;
    // tf->spsr_el1 = 0x0;
}


u64 ustack_alloc(pcb_t* proc, u64 argv){
    // 2 MiB stack
    u32 stack_size = PAGE_SIZE << 9;

    // create a new VMA for the process stack
    vma* stack_vma = kmalloc(sizeof(vma));
    stack_vma->start = USER_STACK_TOP - stack_size;
    stack_vma->end = USER_STACK_TOP;

    // make it RW
    stack_vma->permissions = (VMA_READ | VMA_WRITE);

    list_add(&stack_vma->list, &proc->vmas);

    // allocate a physical page for the argument list
    u64 page = buddy_alloc(PAGE_SIZE);

    // BUG: maybe should use memzero idk
    memset((void*) page, 0, PAGE_SIZE);
    map(USER_STACK_TOP - PAGE_SIZE, va_to_pa(page), 0, MAP_USER | stack_vma->permissions, proc->ttbr);

    // resolve the arguments and copy them into the stack
    char** args = (char**) argv;
    int argc = 0;
    int total_arg_bytes = 0;
    if(args){
        while(*args){
            INFO("ARG %d: %s\n", argc, *args);

            // plus one per null
            total_arg_bytes += strlen(*args) + 1;
            args++;
            argc++;
        }
    }

    // argc = 1 slot, argv = 1 + num_args, envp = 1, at_pagesz = 1, page_size = 1, AT_NULL = 1, 
    u64 nslots = 7 + (argc);

    u64* s = page + PAGE_SIZE;
    u64 sp = USER_STACK_TOP;

    u64 kernel_to_user_adjustment = (u64)s - sp;

    // align the number of bytes consumed to 16
    u64 n_bytes_consumed = ALIGN_UP(total_arg_bytes, 16);
    s = UNSCALED_POINTER_SUB(s, n_bytes_consumed);

    u64* argvlink = (u64*) ALIGN_DOWN((u64) UNSCALED_POINTER_SUB(page + PAGE_SIZE, n_bytes_consumed) - (nslots * 8), 16);

    // copy all the strings into the stack
    args = argv;
    u8 args_written = 0;
    if(args){
        while(*args){
            u64 len = strlen(*args) + 1;
            memcpy(s, *args, len);
            argvlink[1 + args_written] = (u64) s - kernel_to_user_adjustment;
            s = UNSCALED_POINTER_ADD(s, len);

            args++;
            args_written++;
        }
    }

    // TODO: Need to make sure that the arguments can all live within the 4KiB of mapped stack memory
    
    // now come the slots
    s = UNSCALED_POINTER_SUB(page + PAGE_SIZE, n_bytes_consumed);
    sp = USER_STACK_TOP - n_bytes_consumed;

    // write through the kernel alias, so this doesn't depend on which TTBR0 is live
    s = (u64*) ALIGN_DOWN((u64) s - (nslots * 8), 16);
    sp = ALIGN_DOWN(sp - (nslots * 8), 16);

    s[0] = argc;          // argc
    s[nslots - 6] = 0;          // argv terminator
    s[nslots - 5] = 0;          // envp terminator
    s[nslots - 4] = 6;          // AT_PAGESZ
    s[nslots - 3] = PAGE_SIZE;
    s[nslots - 2] = 0;          // AT_NULL
    s[nslots - 1] = 0;

    return sp;
}

void check_and_reap(u64* old_kernel_stack){
    // recover the old pcb from the kernel stack pointer
    pcb_t* old_pcb = (pcb_t*) ((char*)(old_kernel_stack) - offsetof(pcb_t, kernel_stack));


    // free the old processes kernel stack if the previous process was terminated
    if(old_pcb->state == PROCESS_TERMINATED){
        // recover the head page from a modified kernel stack pointer
        void* stack_head = head_from_page(ALIGN_DOWN(old_pcb->kernel_stack, PAGE_SIZE));
        buddy_free(stack_head);

        // invalidate the terminated kernel stack pointer
        old_pcb->kernel_stack = NULL;

        // release the old pcb
        kfree(old_pcb);
    }

}