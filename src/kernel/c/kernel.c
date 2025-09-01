#include "macros.h"
#include "printf.h"
#include "irq.h"
#include "utils.h"
#include "timer.h"
#include "gpio.h"
#include "peripherals/gic.h"
#include "scheduler.h"

#define PIN17 17
#define PIN26 26


void debug_init(){

    // JTAG
    // #define TCK 22
    // #define TMS 27
    // #define TDI 4
    // #define TDO 5
    // #define TRST 25

    // gpio_pin_set_func(TCK, GFAlt4);
    // gpio_pin_set_func(TMS, GFAlt4);
    // gpio_pin_set_func(TDI, GFAlt4);
    // gpio_pin_set_func(TDO, GFAlt4);
    // gpio_pin_set_func(TRST, GFAlt4);

    // gpio_pin_enable(TCK);
    // gpio_pin_enable(TMS);
    // gpio_pin_enable(TDI);
    // gpio_pin_enable(TDO);
    // gpio_pin_enable(TRST);


    // SWD
    // #define SWCLK 22
    // #define SWDIO 4
    // #define RESET 25  // Optional

    // gpio_pin_set_func(SWCLK, GFAlt4);
    // gpio_pin_set_func(SWDIO, GFAlt4);
    // gpio_pin_set_func(RESET, GFAlt4);

    // gpio_pin_enable(SWCLK);
    // gpio_pin_enable(SWDIO);
    // gpio_pin_enable(RESET);
}

void hardware_init(){
    printf("Initializing IRQ vector table...\n");
    irq_init_vectors();

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

    printf("Enabling system scheduler...\n");
    scheduler_init();

    printf("Hardware initialization complete.\n\n");
}

int kernel_main(){
    uart_init();
    printf("\nRaspberry PI Baremetal OS Initializing...\n");

    printf("Execution level: %d\n", get_el());

    printf("Enabling debugging via JTAG interface...\n");
    debug_init();
    hardware_init();

    printf("GICD Enanbles[0]: %x\n", REGS_GICD->gicd_isenabler[0]);
    // turn 17 on
    pulse(PIN17, FALSE);
    int intnum = 0;
    while(1){

        // will never do anything ever again - scheduler will drop us into user land
        WFI();
        printf("Intnum: %d\n", intnum++);

    }
}