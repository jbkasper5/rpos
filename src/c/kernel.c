#include "macros.h"
#include "print.h"

int kernel_main(){
    uart_init();
    printf("Raspberry PI Baremetal OS Initializing...\n");

    printf("Initializing kernel for Pi 4B\n");

    printf("Done.\n");
    while(1){
        printf("%c", uart_getc());
    }
}