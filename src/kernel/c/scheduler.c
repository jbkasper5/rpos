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
    gpio_pin_enable(DEBUG_PIN);
    gpio_pin_set_func(DEBUG_PIN, GFOutput);

    // for now, we say there is 1 task, the IDLE task, which is a placeholder for when the CPU is idle
    proclist.processes = 1;

    // first user process program counter will point to the function do user things
    proclist.proclist[0].registers.pc = (uint64_t) &do_user_things;
    proclist.proclist[0].registers.sp = (uint64_t) USTACK;
    // proclist.proclist[0].registers.spsr = 0; // DAIF = 1111, EL0t mode
    active_process = -1;
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
    printf("Doing scheduler stuffs\n");
    prime_physical_timer();

    // if(active_process != 0){
    //     active_process = 0;
    //     reglist_t* new_regs = &proclist.proclist[active_process].registers;

    //     // printf("New context: \n");
    //     // print_reg_file(new_regs);

    //     // printf("Old context: \n");
    //     // print_reg_file(reg_addr);

    //     // copy the context swapped process into the EL1 stack to return to new user context
    //     memcpy((void*)reg_addr, (void*)new_regs, sizeof(reglist_t));

    //     reg_addr->pc = (uint64_t) &do_user_things;
        
    //     // printf("Post-context switch: \n");
    //     // print_reg_file(reg_addr);
    // }
    pulse(DEBUG_PIN, TRUE);
}