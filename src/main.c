#include "macros.h"
#include "uart.h"
#include "print.h"
#include "utils.h"

int kernel_main(int argc, char** argv){
    uart_init();
    printf("Initializing Baremetal kernel...\n");
    printf("Testing, %d\n", 2);

    // right now, boots to EL2
    // should later boot to EL3
    printf("EL: %d\n", get_el());
    while(1) uart_putc(uart_getc());
}