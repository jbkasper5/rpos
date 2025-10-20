#include "macros.h"
#include "printf.h"
#include "irq.h"
#include "utils.h"
#include "timer.h"
#include "gpio.h"
#include "peripherals/gic.h"
#include "scheduler.h"
#include "mmu.h"


void debug_init(){

    // JTAG
    #define TCK 22
    #define TMS 27
    #define TDI 4
    #define TDO 5
    #define TRST 25

    gpio_pin_set_func(TCK, GFAlt4);
    gpio_pin_set_func(TMS, GFAlt4);
    gpio_pin_set_func(TDI, GFAlt4);
    gpio_pin_set_func(TDO, GFAlt4);
    gpio_pin_set_func(TRST, GFAlt4);

    gpio_pin_enable(TCK);
    gpio_pin_enable(TMS);
    gpio_pin_enable(TDI);
    gpio_pin_enable(TDO);
    gpio_pin_enable(TRST);

    int gate = 0;
    printf("Waiting for gate to be released by debugger...\n");
    while(!gate);
}

void hardware_init(){
    printf("Initializing IRQ vector table...\n");
    // irq_init_vectors();

    printf("Enabling interrupt controller...\n");
    enable_interrupt_controller();

    printf("Enabling system timers...\n");
    timer_init();

    printf("Enabling physical timer...\n");
    physical_timer_enable();

    printf("Priming physical timer...\n");
    prime_physical_timer();

    printf("Enabling IRQ interrupts...\n");
    irq_enable();

    printf("Initializing MMU...\n");
    // mmu_init();

    printf("Enabling system scheduler...\n");
    scheduler_init();

    printf("Hardware initialization complete.\n\n");
}


void drop_to_user();

int kernel_main(){
    printf("\nRaspberry PI Baremetal OS Initializing...\n");

    printf("Execution level: %d\n", get_el());

    hardware_init();

    // turn 18 on
    pulse(DEBUG_PIN, FALSE);
    int intnum = 0;
    while(intnum < 10){
        // WFI();
        printf("Intnum: %d\n", intnum++);
        timer_sleep(100);
    }

    printf("Waiting complete, dropping to user mode...\n");
    drop_to_user();

    return 0;
}