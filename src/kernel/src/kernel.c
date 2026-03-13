#include "macros.h"
#include "io/kprintf.h"
#include "system/irq.h"
#include "utils/utils.h"
#include "utils/timer.h"
#include "io/gpio.h"
#include "peripherals/gic.h"
#include "system/scheduler.h"
#include "memory/mmu.h"
#include "mailbox/mailbox.h"
#include "io/lcd.h"
#include "io/cli.h"
#include "emmc/emmc.h"
#include "filesystem/disk.h"
#include "filesystem/filesystem.h"
#include "memory/kmalloc.h"
#include "filesystem/elf.h"

void hardware_init(){

    finish_virtual_mapping();

    // load the font for the CLI
    load_font();

    // INFO("Enabling LCD panel...\n");
    init_framebuffer();

    INFO("Enabling interrupt controller...\n");
    enable_interrupt_controller();

    INFO("Enabling system timers...\n");
    timer_init();

    INFO("Enabling virtual timer...\n");
    virtual_timer_enable();

    // INFO("Priming physical timer...\n");
    // prime_physical_timer();

    INFO("Enabling IRQ interrupts...\n");
    irq_enable();

    INFO("Enabling SD...\n");
    if(!emmc_init()){
        ERROR("SD card initialization failed.\n");
        while(1);
    }

    INFO("Initializing kernel heap...\n");
    kheap_init();

    INFO("Initializing filesystem...\n");
    filesystem_init();

    INFO("Enabling system scheduler...\n");
    scheduler_init();

    INFO("Hardware initialization complete.\n\n");
}

int kernel_main(){
    
    // DEBUG("Raspberry PI Baremetal OS Initializing...\n");
    hardware_init();

    file_t* file = open("/bin/pwd", 0);
    ext4_block* block = (ext4_block*) kmalloc(sizeof(ext4_block));
    if(!file){
        ERROR("Failed to open file.\n");
    }else{
        INFO("Successfully opened file. FP: 0x%x. Starting ELF parsing.\n", file);
        readelf(file);
        close(file);
    }

    start_scheduler();
    return 0;
}

void idle(){
    while(TRUE){
        uart_putc(uart_getc());
    }
}

/*
TODO:
    - Maintain the reference counter
        - To know when pages can be delivered back to the buddy allocator
    - Finish buddy free
*/