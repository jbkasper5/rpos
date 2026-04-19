#include "system/scheduler.h"
#include "utils/timer.h"
#include "io/kprintf.h"
#include "peripherals/gpio.h"
#include "io/gpio.h"
#include "user.h"
#include "memory/mem.h"
#include "memory/mm.h"
#include "memory/mmap.h"

pcb_list_t proclist;
u64 active_process = 0;

static void idle(){
    asm volatile("msr daifclr, #0xf");
    while(TRUE) WFI();
}


void scheduler_init(){
    // create 2 active processes
    proclist.processes = 1;

    // process 0 is the idle proc
    proclist.proclist[0].registers.pc = &idle;
}

void print_reg_file(reglist_t* regfile){
    kprintf("Register file at: 0x%x\n", regfile);
    for(int i = 0; i < 31; i++){
        kprintf("\tx%d: 0x%x\n", i, regfile->regs[i]);
    }
    kprintf("\tsp: 0x%x\n", regfile->sp);
    kprintf("\tpc: 0x%x\n", regfile->pc);
    kprintf("\tspsr: 0x%x\n\n", regfile->spsr);
}

void scheduler(reglist_t* reg_addr){
    // switch active processes
    int selected_process = 0;
    int i = active_process;
    for(int idx = 0; idx < proclist.processes; idx++){

        // handle wraparounds
        i %= proclist.processes;

        // only look for processes that are ready
        if(proclist.proclist[i].state == PROCESS_READY){

            // if a process is ready and it's not the one currently executing, switch immediately
            if(i != active_process){
                selected_process = i;
                break;
            }

        // if we haven't selected a process yet and the current one is still running, select it
        }else if(proclist.proclist[i].state == PROCESS_RUNNING && !selected_process){
            selected_process = i;
        }

        i++;
    }
    // at this point, either we found a new process, the active process hasn't changed, or the selected process
    // is the idle process

    // if the selected process differs from the active process, we need to perform a context switch
    if(selected_process != active_process){
        // backup process state into proclist
        // dest, src, # bytes
        memcpy(&proclist.proclist[active_process].registers, reg_addr, sizeof(reglist_t));

        // if the process was running, change it back to ready
        if(proclist.proclist[active_process].state == PROCESS_RUNNING){
            proclist.proclist[active_process].state = PROCESS_READY;
        }

        // swap in context of new process
        memcpy(reg_addr, &proclist.proclist[selected_process].registers, sizeof(reglist_t));

        // update active process
        active_process = selected_process;

        // set active process to running
        proclist.proclist[active_process].state = PROCESS_RUNNING;
    }

    DEBUG("Scheduled %d\n", active_process);

    // prime the scheduler timer for another quantum
    prime_physical_timer();
}

static u64 get_current_daif() {
    u64 daif;
    asm volatile ("mrs %0, daif" : "=r" (daif));
    return daif;
}

void start_scheduler(){
    // SAFE - start running the idle task and have scheduler switch active process in
    u32 active = 1;
    pcb_t* current = &proclist.proclist[active];

    set_current(current);

    current->state = PROCESS_RUNNING;

    u64 sp = current->registers.sp;
    u64 pc = current->registers.pc;
    u64 spsr = current->registers.spsr;
    u64 ttbr = current->registers.ttbr;
    drop_to_user(sp, pc, spsr, ttbr);
}

void deschedule(){
    // move the current running process to the waiting queue
    pcb_t* current = get_current();

    DEBUG("Descheduling %d\n", current->pid);
}

void reschedule(u64 procnum){
    DEBUG("Rescheduling %d\n", procnum);
    proclist.proclist[procnum].state = PROCESS_READY;
}

void add_to_schedule(pcb_t* proc){
    proc->state = PROCESS_READY;
    proc->registers.spsr = 0x0;
    proc->pid = proclist.processes;
    memcpy(&proclist.proclist[proclist.processes], proc, sizeof(pcb_t));
    proclist.processes++;
}

// placeholder for now
void reap(u64 procnum){
    // deschedule();
    reschedule(procnum);
}

extern TEST_FN void user();
void* user_ptr = (void*)user;

void add_test_section_to_scheduler(){
    pcb_t* proc = procalloc();
    u64 test_phys = get_phys_test_region();
    u64 test_virt = get_virt_test_region();
    u64 test_size = get_test_size();
    u16 order = log2_pow2(test_size / 4096);

    map(test_virt, test_phys, order, MAP_USER | MAP_READ | MAP_EXEC, proc->registers.ttbr);

    proc->registers.pc = user_ptr;

    add_to_schedule(proc);
}