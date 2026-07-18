#include "system/scheduler.h"
#include "utils/timer.h"
#include "io/kprintf.h"
#include "peripherals/gpio.h"
#include "io/gpio.h"
#include "user.h"
#include "memory/mem.h"
#include "memory/mm.h"
#include "memory/mmap.h"
#include "types/kernel_types.h"

static list_head_t proclist;
static list_head_t runqueue;
static u64 n_processes = 0;

static pcb_t* idle_proc;
static pcb_t* progenitor_proc;

#define KSTACK_SIZE PAGE_SIZE

static void idle(){
    u64 i = 0;
    while(TRUE){
        INTERRUPT_ENABLE();
        WFI();
        // INTERRUPT_DISABLE();
        scheduler();
    }
}

static void* initialize_proc_kstack(){
    u64 range = buddy_alloc(KSTACK_SIZE + PAGE_SIZE);
    map(range + PAGE_SIZE, va_to_pa(range + PAGE_SIZE), 0, MAP_KERNEL | MAP_READ | MAP_WRITE, L0_TABLE);   

    return range + (KSTACK_SIZE + PAGE_SIZE) - 0x10;
}

static void pre_context_switch(pcb_t* newproc, void* old_sp_buffer){
    // get new context stack
    u64 new_sp = newproc->kernel_stack;

    // get new base L0 table
    u64 new_ttbr = newproc->ttbr;

    // switch current process
    set_current(newproc);

    // prime the timer
    prime_physical_timer();

    // set process to running
    newproc->state = PROCESS_RUNNING;

    // switch
    context_switch(new_sp, old_sp_buffer, new_ttbr);
}

void scheduler_init(){
    // initialize process list and runqueue
    INIT_LIST_HEAD(&proclist);
    INIT_LIST_HEAD(&runqueue);

    // create the idle process
    idle_proc = (pcb_t*) kmalloc(sizeof(pcb_t));

    idle_proc->kernel_stack = initialize_proc_kstack();
    idle_proc->ttbr = NULL;

    idle_proc->state = PROCESS_READY;

    trap_frame_t* tf = idle_proc->kernel_stack - sizeof(trap_frame_t);

    // process 0 is the idle proc
    tf->elr_el1 = &idle;

    // empty out SP and TTBR since the idle process does exactly nothing
    tf->sp_el0 = NULL;

    // set up the idle proc to run in EL1 with interrupts enabled
    tf->spsr_el1 = 0x345;

    // set the idle process as the current "user" process
    set_current(idle_proc);

    progenitor_proc = (pcb_t*) kmalloc(sizeof(pcb_t));

    progenitor_proc->pid = 1;
}

void scheduler(){
    pcb_t* active_process = get_current();
    pcb_t* newproc;

    /*
    Since the runqueue is READY-only, and it's head insert, per RR rules, 
    the next process to get it's quantum is the tail, i.e. head->prev

    We just need to check first if the current process is the only 
    process left to run, and what it's state is
    */

    // if the runqueue is empty, it means the current process is the last one
    if(list_empty(&runqueue)){

        // just exit early if the process is still running
        if(active_process->state == PROCESS_RUNNING){
            prime_physical_timer();
            return;
        }

        // otherwise, context switch in the idle_proc
        pre_context_switch(idle_proc, &active_process->kernel_stack);
        return;
    }else{

        // runqueue isn't empty, so we get the next process off the queue
        newproc = list_entry(runqueue.next, pcb_t, runqueue);
    }

    // get the current process's kstack pointer
    u64 old_sp_buffer = &active_process->kernel_stack;

    // remove the new process from the runqeue since it's getting scheduled in
    if(newproc->pid != 0) list_remove(&newproc->runqueue);
    if(active_process->state == PROCESS_RUNNING){
        active_process->state = PROCESS_READY;

        // add the active process back to the runqueue since it's getting scheduled out
        if(active_process->pid != 0) list_add(&active_process->runqueue, &runqueue);
    }

    pre_context_switch(newproc, old_sp_buffer);
}

void start_scheduler(){
    // starts as the idle proc
    pcb_t* current = get_current();

    current->state = PROCESS_RUNNING;

    set_current(current);

    trap_frame_t* tf = current->kernel_stack - sizeof(trap_frame_t);

    u64 sp = tf->sp_el0;
    u64 pc = tf->elr_el1;
    u64 spsr = tf->spsr_el1;
    u64 ttbr = current->ttbr;
    drop_to_user(sp, pc, spsr, ttbr, current->kernel_stack);
}

void deschedule(){
    // move the current running process to the waiting queue
    pcb_t* current = get_current();

    // remove the current from the runqueue before invoking the scheduler
    if(current->state == PROCESS_RUNNING){
        current->state = PROCESS_BLOCKED;
        list_remove(&current->runqueue);
    }
    
    scheduler();
}

void reschedule(pcb_t* proc){
    // DEBUG("Rescheduling %d\n", proc->pid);
    proc->state = PROCESS_READY;
    list_add(&proc->runqueue, &runqueue);
}

void add_to_schedule(pcb_t* proc){
    proc->state = PROCESS_READY;
    list_add(&proc->proclist, &proclist);
    list_add(&proc->runqueue, &runqueue);
    n_processes++;
}

// placeholder for now
void reap(){
    pcb_t* current = get_current();

    // mark the process as terminated
    current->state = PROCESS_TERMINATED;

    // TODO: eventually needs to add process to a reap queue for the next process to handle
    pcb_t* parent = current->parent;

    if(parent->waiting_on == current->pid || parent->waiting_on == WAITING_ALL){
        parent->state = PROCESS_READY;
        list_add(&parent->runqueue, &runqueue);
    }

    scheduler();
    // reap the rest of the process, clean up memory, notify any parents/children, etc.
}

extern TEST_FN void user();
void* user_ptr = (void*)user;

void add_test_section_to_scheduler(){
    pcb_t* proc = procalloc((u64) user_ptr);
    u64 test_phys = get_phys_test_region();
    u64 test_virt = get_virt_test_region();
    u64 test_size = get_test_size();
    u16 order = log2_pow2(test_size / 4096);

    map(test_virt, test_phys, order, MAP_USER | MAP_READ | MAP_WRITE | MAP_EXEC, proc->ttbr);

    // write the return pointer into the user stack
    trap_frame_t* tf = proc->kernel_stack - sizeof(trap_frame_t);
    tf->elr_el1 = user_ptr;

    // proc->registers.pc = user_ptr;

    add_to_schedule(proc);
}