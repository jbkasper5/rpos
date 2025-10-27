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
    proclist.processes = 2;

    // first user process program counter will point to the function do user things
    proclist.proclist[0].registers.pc = (uint64_t) &do_user_things;
    proclist.proclist[0].registers.sp = (uint64_t) USTACK;
    proclist.proclist[0].registers.spsr = 0;

    // second user process will point to the other user function
    proclist.proclist[1].registers.pc = (uint64_t) &do_user_things_2;
    proclist.proclist[1].registers.sp = (uint64_t) (USTACK - (1 << 11));
    proclist.proclist[1].registers.spsr = 0;

    active_process = 0;
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
    // static int trigger = 0;
    printf("Doing scheduler stuffs. Active process is: %d\n", active_process);

    printf("Incoming context:\n");
    print_reg_file(reg_addr);

    // backup process state
    // dest, src, # bytes
    memcpy(&proclist.proclist[active_process].registers, reg_addr, sizeof(reglist_t));

    printf("Backed up context:\n");
    print_reg_file(&proclist.proclist[active_process].registers);
    active_process = !active_process;

    // swap in context of new process
    memcpy(reg_addr, &proclist.proclist[active_process].registers, sizeof(reglist_t));
    printf("Returning to new context:\n");
    print_reg_file(reg_addr);

    prime_physical_timer();
    pulse(DEBUG_PIN, TRUE);
}