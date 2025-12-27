#include "system/filesystem.h"
#include "io/printf.h"
#include "macros.h"

void print_partitions(mbr* mbr){
    printf("Parsed MBR: \n");
    for(int i = 0; i < 4; i++){
        printf("  Partition %d: \n", i);
        if(mbr->partitions[i].type){
            printf("    Status:  0x%x\n", mbr->partitions[i].status);
            printf("    Type:    0x%x\n", mbr->partitions[i].type);
            printf("    LBA:     0x%x\n", mbr->partitions[i].first_lba_sector);
            printf("    Sectors: 0x%x\n", mbr->partitions[i].num_sectors);
        }else{
            printf("    EMPTY/UNUSED\n");
        }
    }
    printf("END MBR.\n");
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
        for(int i = 0; i < 8; i++) printf("%c", dir->filename[i]);
        printf(" | ");
        for(int i = 0; i < 3; i++) printf("%c", dir->extension[i]);
        printf(" | ");
        if(dir->attributes & 0x08){
            printf("VOLUME LABEL\n");
        }else{
            printf("ATTRIBUTES: 0x%x\n", dir->attributes);
        }
        dir++;
    }
}
