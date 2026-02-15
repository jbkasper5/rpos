#include "system/scheduler.h"
#include "utils/timer.h"
#include "io/kprintf.h"
#include "peripherals/gpio.h"
#include "io/gpio.h"
#include "user.h"
#include "memory/mem.h"
#include "memory/mm.h"

extern reglist_t* user_context_ptr;

pcb_list_t proclist;
uint64_t active_process = 0;

void scheduler_init(){
    // debug pins
    gpio_pin_enable(USER_PIN);
    gpio_pin_set_func(USER_PIN, GFOutput);
    gpio_pin_enable(USER_PIN_2);
    gpio_pin_set_func(USER_PIN_2, GFOutput);
    gpio_pin_enable(DEBUG_PIN);
    gpio_pin_set_func(DEBUG_PIN, GFOutput);

    // create 2 active processes
    proclist.processes = 0;

    // idle process, for when no real user processes are available
    // proclist.proclist[0].registers.pc = (uint64_t) &idle_proc;
    // proclist.proclist[0].registers.sp = (uint64_t) (USTACK - (1 << 11) + 128);
    // proclist.proclist[0].registers.spsr = 0;
    // proclist.proclist[0].state = PROCESS_READY;

    // first user process program counter will point to the function do user things
    // proclist.processes++;
    // proclist.proclist[1].registers.pc = (uint64_t) &do_user_things;
    // proclist.proclist[1].registers.sp = (uint64_t) USTACK;
    // proclist.proclist[1].registers.spsr = 0;
    // proclist.proclist[1].state = PROCESS_READY;

    // // second user process will point to the other user function
    // proclist.processes++;
    // proclist.proclist[2].registers.pc = (uint64_t) &do_user_things_2;
    // proclist.proclist[2].registers.sp = (uint64_t) (USTACK - (1 << 11));
    // proclist.proclist[2].registers.spsr = 0;
    // proclist.proclist[2].state = PROCESS_READY;

    // third user process will lead to assembly for testing purposes
    // proclist.processes++;
    // proclist.proclist[3].registers.pc = (uint64_t) &do_user_things_2;
    // proclist.proclist[3].registers.sp = (uint64_t) (USTACK - (1 << 11));
    // proclist.proclist[3].registers.spsr = 0;
    // proclist.proclist[3].state = PROCESS_READY;
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
    pulse(DEBUG_PIN, FALSE);

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
    pulse(DEBUG_PIN, TRUE);
}

void start_scheduler(){
    // SAFE - start running the idle task and have scheduler switch active process in
    active_process = 0;
    proclist.proclist[active_process].state = PROCESS_RUNNING;

    uint64_t sp = proclist.proclist[active_process].registers.sp;
    uint64_t pc = proclist.proclist[active_process].registers.pc;
    uint64_t spsr = proclist.proclist[active_process].registers.spsr;
    uint64_t ttbr = proclist.proclist[active_process].registers.ttbr;
    drop_to_user(sp, pc, spsr, ttbr);
}

void deschedule(){
    // move the current running process to the waiting queue
    proclist.proclist[active_process].state = PROCESS_BLOCKED;

    DEBUG("Descheduling %d\n", active_process);

    // DEBUG("Context file saved for process %d: \n", active_process);
    // print_reg_file(user_context_ptr);

    // reschedule now that we changed the state of things
    scheduler(user_context_ptr);
}

void reschedule(uint64_t procnum){
    DEBUG("Rescheduling %d\n", procnum);
    proclist.proclist[procnum].state = PROCESS_READY;
    scheduler(user_context_ptr);
}

void add_to_schedule(pcb_t* proc){
    proc->state = PROCESS_READY;
    proc->registers.spsr = 0x0;
    memcpy(&proclist.proclist[proclist.processes], proc, sizeof(pcb_t));
    proclist.processes++;
}