#include "macros.h"
#include "print.h"
#include "irq.h"
#include "utils.h"
#include "timer.h"

int kernel_main(){
    uart_init();
    printf("\nRaspberry PI Baremetal OS Initializing...\n");

    irq_init_vectors();
    enable_interrupt_controller();
    irq_enable();
    timer_init();

    printf("\tInitializing kernel for Pi 4B\n");

    printf("Done.\n");
    printf("\nExecution level: %d\n", get_el());

    printf("Sleeping 200 ms...\n");
    timer_sleep(200);

    printf("Sleeping 2 s...\n");
    timer_sleep(2000);

    printf("DONE!\n");
    while(1){
        // printf("%c", uart_getc());
    }
}