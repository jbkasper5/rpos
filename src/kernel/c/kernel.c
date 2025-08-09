#include "macros.h"
#include "print.h"
#include "irq.h"
#include "utils.h"
#include "timer.h"
#include "i2c.h"
#include "spi.h"

void hardware_init(){
    printf("Initializing IRQ vector table...\n");
    irq_init_vectors();

    printf("Enabling interrupt controller...\n");
    enable_interrupt_controller();

    printf("Enabling IRQ interrupts...\n");
    irq_enable();

    printf("Enabling system timers...\n");
    timer_init();

    printf("Enabling I2C communication...\n");
    i2c_init();

    printf("Enabling SPI communication...\n");
    spi_init();

    printf("Hardware initialization complete.\n\n");
}

int kernel_main(){
    uart_init();
    printf("\nRaspberry PI Baremetal OS Initializing...\n");
    hardware_init();

    // I2C stuffs to arduino board
    /*
    char buf[10];
    for(int i = 0; i < 10; i++){
        ic2_receive(21, buffer, 9);
        buffer[9] = 0
        printf("Received '%s'\n", buffer);
        timer_sleep(250);
    }

    for(uint8_t i = 0; i < 20; i++){
        i2c_send(21, &i, 1);
        timer_sleep(250);
        printf("Sent '%d'\n", i);
    }
    char* message = "Hello slave device";
    i2c_send(21, message, 18);
    
    */

    while(1){
        printf("%c", uart_getc());
    }
}