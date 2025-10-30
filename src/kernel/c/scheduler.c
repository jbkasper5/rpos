#include "scheduler.h"
#include "timer.h"
#include "printf.h"
#include "peripherals/gpio.h"
#include "gpio.h"
#include "user.h"
#include "mem.h"
#include "mm.h"

pcb_list_t proclist;
uint64_t active_process;

void scheduler_init(){
    // debug pins
    gpio_pin_enable(USER_PIN);
    gpio_pin_set_func(USER_PIN, GFOutput);
    gpio_pin_enable(USER_PIN_2);
    gpio_pin_set_func(USER_PIN_2, GFOutput);
    gpio_pin_enable(DEBUG_PIN);
    gpio_pin_set_func(DEBUG_PIN, GFOutput);

    // creare 2 active processes
    proclist.processes = 1;

    // idle process, for when no real user processes are available
    proclist.proclist[0].registers.pc = (uint64_t) &idle_proc;
    proclist.proclist[0].registers.sp = (uint64_t) (USTACK - (1 << 11) + 128);
    proclist.proclist[0].registers.spsr = 0;
    proclist.proclist[0].state = PROCESS_READY;

    // first user process program counter will point to the function do user things
    proclist.processes++;
    proclist.proclist[1].registers.pc = (uint64_t) &do_user_things;
    proclist.proclist[1].registers.sp = (uint64_t) USTACK;
    proclist.proclist[1].registers.spsr = 0;
    proclist.proclist[1].state = PROCESS_BLOCKED;

    // second user process will point to the other user function
    proclist.processes++;
    proclist.proclist[2].registers.pc = (uint64_t) &do_user_things_2;
    proclist.proclist[2].registers.sp = (uint64_t) (USTACK - (1 << 11));
    proclist.proclist[2].registers.spsr = 0;
    proclist.proclist[2].state = PROCESS_READY;
}

void print_reg_file(reglist_t* regfile){
    printf("Register file at: 0x%x\n", regfile);
    for(int i = 0; i < 31; i++){
        printf("\tx%d: 0x%x\n", i, regfile->regs[i]);
    }
    printf("\tsp: 0x%x\n", regfile->sp);
    printf("\tpc: 0x%x\n", regfile->pc);
    printf("\tspsr: 0x%x\n\n", regfile->spsr);
}

void scheduler(reglist_t* reg_addr){
    pulse(DEBUG_PIN, FALSE);

    // switch active processes
    int selected_process = 0;
    for(int i = 1; i < proclist.processes; i++){
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
    }
    // at this point, either we found a new process, the active process hasn't changed, or the selected process
    // is the idle process

    // if the selected process differs from the active process, we need to perform a context switch
    if(selected_process != active_process){
        // backup process state into proclist
        // dest, src, # bytes
        memcpy(&proclist.proclist[active_process].registers, reg_addr, sizeof(reglist_t));

        // change the running process to ready instead of running
        proclist.proclist[active_process].state = PROCESS_READY;

        // swap in context of new process
        memcpy(reg_addr, &proclist.proclist[selected_process].registers, sizeof(reglist_t));

        // update active process
        active_process = selected_process;

        // set active process to running
        proclist.proclist[active_process].state = PROCESS_RUNNING;
    }

    // prime the scheduler timer for another quantum
    prime_physical_timer();
    pulse(DEBUG_PIN, TRUE);
}

void start_scheduler(){
    active_process = 2;
    proclist.proclist[active_process].state = PROCESS_RUNNING;

    uint64_t sp = proclist.proclist[active_process].registers.sp;
    uint64_t pc = proclist.proclist[active_process].registers.pc;
    uint64_t spsr = proclist.proclist[active_process].registers.spsr;
    drop_to_user(sp, pc, spsr);
}

void deschedule(){
    // move the current running process to the waiting queue
}