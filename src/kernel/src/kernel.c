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

    INFO("Enabling system scheduler...\n");
    scheduler_init();

    INFO("Enabling SD...\n");
    if(!emmc_init()){
        ERROR("SD card initialization failed.\n");
        while(1);
    }

    INFO("Initializing kernel heap...\n");
    kheap_init();

    INFO("Initializing filesystem...\n");
    filesystem_init();

    INFO("Initializing device drivers...\n");
    device_tree_init();

    INFO("Enabling physical timer...\n");
    physical_timer_enable();

    INFO("Enabling virtual timer...\n");
    virtual_timer_enable();

    INFO("Priming physical timer...\n");
    prime_physical_timer();

    INFO("Enabling IRQ interrupts...\n");
    irq_enable();

    INFO("Hardware initialization complete.\n\n");
}


static u64 get_current_daif() {
    u64 daif;
    asm volatile ("mrs %0, daif" : "=r" (daif));
    return daif;
}


#include "system/gic.h"

int kernel_main(){
    
    // DEBUG("Raspberry PI Baremetal OS Initializing...\n");
    hardware_init();

    // mailbox_pcie_usb_power_on();

    // file_t* file = open("/bin/pwd", 0);
    // ext4_block* block = (ext4_block*) kmalloc(sizeof(ext4_block));
    // if(!file){
    //     ERROR("Failed to open file.\n");
    // }else{
    //     INFO("Successfully opened file. FP: 0x%x. Starting ELF parsing.\n", file);
    //     readelf(file);
    //     close(file);
    // }
    
    // while(TRUE){
    //     uart_putc(uart_getc());
    // }

    add_test_section_to_scheduler(); 
    start_scheduler();

    return 0;
}

/*
TODO:
    - Maintain the reference counter
        - To know when pages can be delivered back to the buddy allocator
    - Finish buddy free
*/


// 0xFFFF800000000000
// 0xFFFF8000FD500000


/*
100s of temple OS
*/