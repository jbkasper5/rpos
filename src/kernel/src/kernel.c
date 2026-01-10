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
#include "system/filesystem.h"
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
    PDEBUG("Waiting for gate to be released by debugger...\n");
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

    INFO("Initializing kernel heap...\n");
    kheap_init();

    INFO("Enabling SD...\n");
    if(!emmc_init()){
        ERROR("SD card initialization failed.\n");
        while(1);
    }

    // enable the font
    INFO("Hardware initialization complete.\n\n");
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

    sector s;
    kprintf("Parsing sector 0x4000...\n");
    int result = emmc_read((uint8_t*) &s.data, sizeof(s));
    if(result){
        kprintf("Sector parsed.\n");
    }else{
        kprintf("ERROR: Error parsing sector.\n");
    }

    // should be where the boot filesystem is written
    emmc_seek_sector(18460);
    kprintf("Parsing sector 18460...\n");
    result = emmc_read((uint8_t*) &s.data, sizeof(s));
    if(result){
        kprintf("Sector parsed.\n");
    }else{
        kprintf("ERROR: Error parsing sector.\n");
    }

    print_fat32_directory(&s);
    emmc_seek_sector(18461);
    result = emmc_read((uint8_t*) &s.data, sizeof(s));
    print_fat32_directory(&s);


    sector s2[2];
    // seek to the user ext superblock
    emmc_seek_sector(0x104002);
    result = emmc_read((uint8_t*)s2, sizeof(sector) * 2);

    // read the sectors as a superblock
    ext_superblock* sb = (ext_superblock*) s2;

    kprintf("\nParsing ext4 filesystem...\n");
    kprintf("SUPERBLOCK VOLUME: %s\n", sb->s_volume_name);
    kprintf("SUPERBLOCK MAGIC: 0x%x\n", sb->s_magic);
    kprintf("BLOCK SIZE: 0x%x\n", 1024 << sb->s_log_block_size);
    kprintf("INODE SIZE: 0x%x\n", sb->s_inode_size);
    kprintf("INODES PER BLOCK: %d\n", (1024 << sb->s_log_block_size) / sb->s_inode_size);
    
    // each block is 8 sectors, so this should be block 1
    emmc_seek_sector(0x104008);
    ext4_block b;
    emmc_read((uint8_t*)&b, sizeof(ext4_block));

    bgdt* doober = (bgdt*) &b.data;

    uint64_t inode_start_sector = (0x104000) + (doober->bg_inode_table * 8);

    emmc_seek_sector(inode_start_sector);
    emmc_read((uint8_t*)&b, sizeof(ext4_block));
    ext4_inode* inode = (ext4_inode*)&b;

    uint64_t root_dir_start_sector = (0x104000) + (inode[1].i_block[0] * 8);

    emmc_seek_sector(root_dir_start_sector);
    emmc_read((uint8_t*)&b, sizeof(ext4_block));

    ext4_dir_entry* dir = (ext4_dir_entry*) &b;
    char fname[dir->name_len + 1];
    memcpy(fname, dir->name, dir->name_len);
    fname[dir->name_len] = '\0';

    kprintf("\n\n> ls\n");

    uint32_t offset = 0;
    while (offset < 4096) {
        ext4_dir_entry *entry = UNSCALED_POINTER_ADD(dir, offset);

        // skip unused entries
        if (entry->inode == 0) {
            offset += entry->rec_len;
            continue;
        }

        // read the filename safely
        char name[256];
        if (entry->name_len >= sizeof(name)) {
            // truncate if too long
            memcpy(name, entry->name, sizeof(name) - 1);
            name[sizeof(name)-1] = '\0';
        } else {
            memcpy(name, entry->name, entry->name_len);
            name[entry->name_len] = '\0';
        }

        // process the entry
        kprintf("%s\n", name);

        // move to next entry
        offset += entry->rec_len;
    }

    kprintf("\n> cat test1.txt\n");

    const char *target = "test1.txt";

    offset = 0;
    uint32_t inode_num = 0;
    while (offset < 4096) {
        ext4_dir_entry *entry = UNSCALED_POINTER_ADD(dir, offset);

        if (entry->inode != 0 && entry->name[0] == target[0] && entry->name[4] == target[4]) {
            // found the file
            inode_num = entry->inode;
            break;
        }

        offset += entry->rec_len;
    }

    char contents[256];
    if (inode_num > 0){
        // reload inode table
        emmc_seek_sector(inode_start_sector);
        emmc_read((uint8_t*)&b, sizeof(ext4_block));
        ext4_inode* inode = (ext4_inode*)&b;
        uint64_t blocknum = (0x104000) + (inode[inode_num - 1].i_block[0] * 8);

        // read the block
        emmc_seek_sector(blocknum);
        emmc_read((uint8_t*) &b, sizeof(ext4_block));
        memcpy(contents, &b, 255);
        contents[255] = '\0';
        kprintf("%s\n", contents);
    }

    while(TRUE){
        uart_putc(uart_getc());
    }

    PDEBUG("Waiting complete, dropping to user mode...\n");
    start_scheduler();

    return 0;
}

/*

p /x *(struct AuxRegs*) 0xFE215000

// before MMU
$1 = {irq_status = 0x0, enables = 0x1, reserved = {0x61757830, 0x61757830, 
    0x61757830, 0x61757830, 0x61757830, 0x61757830, 0x0, 0x1, 0x61757830, 
    0x61757830, 0x61757830, 0x61757830, 0x61757830, 0x61757830}, mu_io = 0x0, 
  mu_ier = 0x1, mu_iir = 0xc3, mu_lcr = 0x3, mu_mcr = 0x0, mu_lsr = 0x60, 
  mu_msr = 0x10, mu_scratch = 0x0, mu_control = 0x3, mu_status = 0x34e, 
  mu_baud_rate = 0x21d}

// after mmu
$2 = {irq_status = 0x0, enables = 0x0, reserved = {0x0, 0x0, 0x61757830, 
    0x61757830, 0x61757830, 0x61757830, 0x0, 0x0, 0x0, 0x0, 0x61757830, 
    0x61757830, 0x61757830, 0x61757830}, mu_io = 0x0, mu_ier = 0x0, mu_iir = 0x0, 
  mu_lcr = 0x0, mu_mcr = 0x0, mu_lsr = 0x0, mu_msr = 0x0, mu_scratch = 0x0, 
  mu_control = 0x3, mu_status = 0x3, mu_baud_rate = 0x3}

*/

/*
start of frame buffer: 0x3ea83000
end of frame buffer:   0x3eae0bff

*/

// 0x69bcb8