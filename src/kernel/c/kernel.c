#include "macros.h"
#include "printf.h"
#include "irq.h"
#include "utils.h"
#include "timer.h"
#include "gpio.h"
#include "peripherals/gic.h"
#include "scheduler.h"
#include "mmu.h"
#include "mailbox.h"
#include "lcd.h"
#include "sdcard.h"


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
    PDEBUG("Waiting for gate to be released by debugger...\n");
    while(!gate);
}

void hardware_init(){

    printf("Enabling interrupt controller...\n");
    enable_interrupt_controller();

    printf("Enabling system timers...\n");
    timer_init();

    printf("Enabling physical timer...\n");
    physical_timer_enable();

    printf("Enabling virtual timer...\n");
    virtual_timer_enable();

    printf("Priming physical timer...\n");
    prime_physical_timer();

    printf("Enabling IRQ interrupts...\n");
    irq_enable();

    // TODO: need to set up VM for the framebuffer
    // printf("Initializing MMU...\n");
    // mmu_init();

    printf("Enabling system scheduler...\n");
    scheduler_init();

    printf("Enabling LCD panel...\n");
    init_framebuffer();

    printf("Hardware initialization complete.\n\n");
}


void drop_to_user();

int kernel_main(){
    PDEBUG("\nRaspberry PI Baremetal OS Initializing...\n");

    PDEBUG("Execution level: %d\n", get_el());

    hardware_init();

    init_sd();
    read_sd();
    while(TRUE);

    PDEBUG("Waiting complete, dropping to user mode...\n");
    start_scheduler();

    return 0;
}