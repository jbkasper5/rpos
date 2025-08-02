#include "macros.h"
#include "print.h"
#include "irq.h"
#include "utils.h"

int kernel_main(){
    uart_init();
    printf("\nRaspberry PI Baremetal OS Initializing...\n");

    irq_init_vectors();
    enable_interrupt_controller();
    irq_enable();

    printf("\tInitializing kernel for Pi 4B\n");

    printf("Done.\n");
    printf("\nExecution level: %d\n", get_el());
    while(1){
        // printf("%c", uart_getc());
    }
}