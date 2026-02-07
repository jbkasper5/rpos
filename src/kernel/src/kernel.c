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
    DEBUG("Waiting for gate to be released by debugger...\n");
    while(!gate);
}

void hardware_init(){

    // load the font for the CLI
    load_font();

    INFO("Enabling LCD panel...\n");
    init_framebuffer();

    INFO("Enabling interrupt controller...\n");
    enable_interrupt_controller();

    INFO("Enabling system timers...\n");
    timer_init();

    // INFO("Enabling physical timer...\n");
    // physical_timer_enable();

    INFO("Enabling virtual timer...\n");
    virtual_timer_enable();

    INFO("Priming physical timer...\n");
    prime_physical_timer();

    INFO("Enabling IRQ interrupts...\n");
    irq_enable();

    INFO("Initializing MMU...\n");
    mmu_init();

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

    INFO("Hardware initialization complete.\n\n");
}

void drop_to_user();

int kernel_main(){
    DEBUG("\nRaspberry PI Baremetal OS Initializing...\n");
    hardware_init();

    file_t* file = open("/bin/ls", 0);
    ext4_block* block = (ext4_block*) kmalloc(sizeof(ext4_block));
    if(!file){
        ERROR("Failed to open /bin/ls\n");
    }else{
        INFO("Successfully opened /bin/ls. FP: 0x%x\n", file);
        uint64_t bytes_read = read(file, block, sizeof(ext4_block));
        INFO("Read %d bytes from /bin/ls.\n", bytes_read);
    }
    
    while(TRUE){
        uart_putc(uart_getc());
    }

    DEBUG("Waiting complete, dropping to user mode...\n");
    start_scheduler();

    return 0;
}

/*
TODO:
    - Maintain the reference counter
        - To know when pages can be delivered back to the buddy allocator
    - Finish buddy free
*/