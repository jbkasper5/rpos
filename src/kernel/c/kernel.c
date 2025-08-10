#include "macros.h"
#include "print.h"
#include "irq.h"
#include "utils.h"
#include "timer.h"
#include "i2c.h"
#include "spi.h"
#include "mailbox.h"

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

    printf("CORE CLOCK: %d\n", mailbox_clock_rate(CT_CORE));
    printf("EMMC CLOCK: %d\n", mailbox_clock_rate(CT_EMMC));
    printf("UART CLOCK: %d\n", mailbox_clock_rate(CT_UART));
    printf("ARM CLOCK: %d\n", mailbox_clock_rate(CT_ARM));

    printf("I2C POWER STATE: \n");

    for(int i = 0; i < 3; i++){
        printf("POWER DOMAIN STATUS FOR %d=%d\n", i, mailbox_power_check(i));
    }

    uint32_t max_temp = 0;
    mailbox_generic_command(RPI_FIRMWARE_GET_MAX_TEMPERATURE, 0, &max_temp);

    while(1){
        uint32_t current_temp = 0;
        mailbox_generic_command(RPI_FIRMWARE_GET_TEMPERATURE, 0, &current_temp);
        printf("CURRENT CPU TEMP: %dC, MAX TEMP: %dC\n", current_temp / 1000, max_temp / 1000);

        // sleep 1 second
        timer_sleep(1000);
    }
}