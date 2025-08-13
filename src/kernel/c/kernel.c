#include "macros.h"
#include "printf.h"
#include "irq.h"
#include "utils.h"
#include "timer.h"

void hardware_init(){
    printf("Initializing IRQ vector table...\n");
    irq_init_vectors();

    printf("Enabling interrupt controller...\n");
    enable_interrupt_controller();

    printf("Enabling IRQ interrupts...\n");
    irq_enable();

    printf("Enabling system timers...\n");
    timer_init();

    printf("Hardware initialization complete.\n\n");
}

int kernel_main(){
    uart_init();
    printf("\nRaspberry PI Baremetal OS Initializing...\n");
    hardware_init();
    
    while(1){
        uart_putc(uart_getc());
    }
}