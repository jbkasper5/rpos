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

    sector s;
    printf("Parsing sector 0x4000...\n");
    int result = emmc_read((uint8_t*) &s.data, sizeof(s));
    if(result){
        printf("Sector parsed.\n");
    }else{
        printf("ERROR: Error parsing sector.\n");
    }

    // should be where the boot filesystem is written
    emmc_seek_sector(18460);
    printf("Parsing sector 18460...\n");
    result = emmc_read((uint8_t*) &s.data, sizeof(s));
    if(result){
        printf("Sector parsed.\n");
    }else{
        printf("ERROR: Error parsing sector.\n");
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

    printf("\nParsing ext4 filesystem...\n");
    PDEBUG("SUPERBLOCK VOLUME: %s\n", sb->s_volume_name);
    PDEBUG("SUPERBLOCK MAGIC: 0x%x\n", sb->s_magic);
    PDEBUG("BLOCK SIZE: 0x%x\n", 1024 << sb->s_log_block_size);
    PDEBUG("INODE SIZE: 0x%x\n", sb->s_inode_size);
    PDEBUG("INODES PER BLOCK: %d\n", (1024 << sb->s_log_block_size) / sb->s_inode_size);
    
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

    printf("\n\n> ls\n");

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
        printf("%s\n", name);

        // move to next entry
        offset += entry->rec_len;
    }

    printf("\n> cat test1.txt\n");

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
        printf("%s\n", contents);
    }

    while(TRUE){
        uart_putc(uart_getc());
    }

    PDEBUG("Waiting complete, dropping to user mode...\n");
    start_scheduler();

    return 0;
}