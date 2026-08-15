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
#include "drivers/dt.h"
#include "drivers/kbd.h"
#include "memory/memprofiler.h"

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

    INFO("Enabling SD...\n");
    if(!emmc_init()){
        ERROR("SD card initialization failed.\n");
        while(1);
    }

    INFO("Initializing kernel heap...\n");
    kheap_init();

    INFO("Enabling system scheduler...\n");
    scheduler_init();

    INFO("Initializing filesystem...\n");
    filesystem_init();

    INFO("Initializing device drivers...\n");
    device_tree_init();

    INFO("Initializing keyboard handler...\n");
    keyboard_init();

    INFO("Enabling physical timer...\n");
    physical_timer_enable();

    INFO("Enabling virtual timer...\n");
    virtual_timer_enable();

    INFO("Priming physical timer...\n");
    prime_physical_timer();

    // INFO("Enabling IRQ interrupts...\n");
    // irq_enable();

    INFO("Hardware initialization complete.\n\n");
}

int kernel_main(){
    panic();
    hardware_init();
    add_test_section_to_scheduler(); 
    start_scheduler();
    return 0;
}