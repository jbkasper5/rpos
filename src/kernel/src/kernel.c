#include "macros.h"
#include "io/printf.h"
#include "system/irq.h"
#include "utils/utils.h"
#include "utils/timer.h"
#include "io/gpio.h"
#include "peripherals/gic.h"
#include "system/scheduler.h"
#include "memory/mmu.h"
#include "mailbox/mailbox.h"
#include "io/lcd.h"
#include "io/fonttest.h"
#include "emmc/emmc.h"
#include "system/filesystem.h"

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

    test_font();

    printf("Enabling LCD panel...\n");
    init_framebuffer();


    printf("Enabling interrupt controller...\n");
    enable_interrupt_controller();

    printf("Enabling system timers...\n");
    timer_init();

    // printf("Enabling physical timer...\n");
    // physical_timer_enable();

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

    printf("Enabling SD...\n");
    if(!emmc_init()){
        printf("ERROR: SD card initialization failed.\n");
        while(1) uart_putc(uart_getc());
    }

    // enable the font
    printf("Hardware initialization complete.\n\n");
}


void drop_to_user();

int kernel_main(){
    PDEBUG("\nRaspberry PI Baremetal OS Initializing...\n");

    PDEBUG("Execution level: %d\n", get_el());

    hardware_init();

    PDEBUG("Reading sd card...\n");
    mbr mbr;
    emmc_read((uint8_t*) &mbr, sizeof(mbr));
    print_partitions(&mbr);

    // seek to start sector of the boot partition
    emmc_seek_sector(0x4000);

    sector sector;
    printf("Parsing sector 0x4000...\n");
    int result = emmc_read(&sector.data, sizeof(sector));
    if(result){
        printf("Sector parsed.\n");
    }else{
        printf("ERROR: Error parsing sector.\n");
    }

    // should be where the boot filesystem is written
    emmc_seek_sector(18460);
    printf("Parsing sector 18460...\n");
    result = emmc_read(&sector.data, sizeof(sector));
    if(result){
        printf("Sector parsed.\n");
    }else{
        printf("ERROR: Error parsing sector.\n");
    }

    print_directory(&sector);
    emmc_seek_sector(18461);
    result = emmc_read(&sector.data, sizeof(sector));
    print_directory(&sector);

    while(TRUE){
        uart_putc(uart_getc());
    }

    PDEBUG("Waiting complete, dropping to user mode...\n");
    start_scheduler();

    return 0;
}