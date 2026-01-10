#include "system/filesystem.h"
#include "io/kprintf.h"
#include "macros.h"

void print_partitions(mbr* mbr){
    kprintf("Parsed MBR: \n");
    for(int i = 0; i < 4; i++){
        kprintf("  Partition %d: \n", i);
        if(mbr->partitions[i].type){
            kprintf("    Status:  0x%x\n", mbr->partitions[i].status);
            kprintf("    Type:    0x%x\n", mbr->partitions[i].type);
            kprintf("    LBA:     0x%x\n", mbr->partitions[i].first_lba_sector);
            kprintf("    Sectors: 0x%x\n", mbr->partitions[i].num_sectors);
        }else{
            kprintf("    EMPTY/UNUSED\n");
        }
    }
    kprintf("END MBR.\n");
}

void print_fat32_directory(sector* s){
    fat32_directory_entry* dir = (fat32_directory_entry*) s->data;
    
    for(int i = 0; i < 16; i++){
        if(!(*((char*) dir))) break;

        // 0xE5 means deleted entry
        if((*((char*) dir)) == 0xE5 || dir->attributes == 0xF){
            dir++;
            continue;
        }
        for(int i = 0; i < 8; i++) kprintf("%c", dir->filename[i]);
        kprintf(" | ");
        for(int i = 0; i < 3; i++) kprintf("%c", dir->extension[i]);
        kprintf(" | ");
        if(dir->attributes & 0x08){
            kprintf("VOLUME LABEL\n");
        }else{
            kprintf("ATTRIBUTES: 0x%x\n", dir->attributes);
        }
        dir++;
    }
}
