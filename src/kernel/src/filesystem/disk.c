#include "filesystem/disk.h"
#include "io/kprintf.h"
#include "macros.h"
#include "memory/mem.h"

fs rootfs;


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

#define FS_START_SECTOR 0x104000
#define ROOT_SUPERBLOCK_SECTOR 0x104002
#define GROUP_DESC_SECTOR 0x104008

static size_t block_size;
static size_t inode_size;
static size_t inodes_per_group;
static size_t gc_descriptor_size;

static ext4_group_desc_t* group_descriptor_table;

static void print_dir(ext4_dir_entry* dir){
    uint32_t offset = 0;
    while (offset < 4096) {
        ext4_dir_entry *entry = (char*)dir + offset;

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
        INFO("%s\n", name);

        // move to next entry
        offset += entry->rec_len;
    }
}

static size_t inode_to_sector(size_t inode_num, size_t* offset){
    uint32_t group = (inode_num - 1) / inodes_per_group;
    uint32_t index = (inode_num - 1) % inodes_per_group;

    uint64_t inode_offset = index * inode_size;
    uint64_t inode_block_offset = inode_offset / block_size;
    uint64_t inode_offset_in_block = inode_offset % block_size;

    *offset = inode_offset_in_block;

    ext4_group_desc_t* ptr = group_descriptor_table + group;
    uint32_t inode_table_block = ptr->bg_inode_table_lo;
    size_t inode_sector = FS_START_SECTOR + ((inode_table_block + inode_block_offset) * 8);

    return inode_sector;

    // emmc_seek_sector(inode_sector);
    // emmc_read(block_buf, sizeof(ext4_block));
    // ext4_inode* inode = (ext4_inode*) UNSCALED_POINTER_ADD(block_buf, inode_offset_in_block);

    // // ignore for now -> but do necessary inode iblock parsing
    // uint32_t dirblock = inode->i_block[0];
    // uint32_t dirsector = FS_START_SECTOR + dirblock * 8;
}   


/**
 * @brief Reads the superblock and returns the sector of the inode table
 * @return Sector of the inode table
 */
static size_t read_superblock(){
    rootfs.block_buf = (ext4_block*) kmalloc(sizeof(ext4_block));
    rootfs.superblock = (ext_superblock*) kmalloc(sizeof(sector) * 2);

    emmc_seek_sector(ROOT_SUPERBLOCK_SECTOR);

    int result = emmc_read(rootfs.superblock, sizeof(sector) * 2);

    ext_superblock* sb = (ext_superblock*) rootfs.superblock;
    block_size = 1024 << sb->s_log_block_size;
    inode_size = sb->s_inode_size;
    inodes_per_group = sb->s_inodes_per_group;
    gc_descriptor_size = sb->s_desc_size ? sb->s_desc_size : 32;


    // read the sectors as a superblock
    INFO("Parsing ext4 filesystem...\n");
    INFO("SUPERBLOCK VOLUME: \e[36m%s\e[0m\n", sb->s_volume_name);
    INFO("SUPERBLOCK MAGIC: 0x%x\n", sb->s_magic);
    INFO("BLOCK SIZE: 0x%x\n", block_size);
    INFO("INODE SIZE: 0x%x\n", inode_size);
    INFO("INODES PER BLOCK: %d\n", block_size / inode_size);

    // each block is 8 sectors, so this should be block 1
    emmc_seek_sector(GROUP_DESC_SECTOR);
    group_descriptor_table = (ext4_group_desc_t*) kmalloc(sizeof(ext4_block) * 8);
    emmc_read((uint8_t*)group_descriptor_table, sizeof(ext4_block) * 8);

    // 8 sectors per block
    uint64_t inode_start_sector = (FS_START_SECTOR) + (group_descriptor_table->bg_inode_table_lo * 8);

    emmc_seek_sector(inode_start_sector);
    emmc_read((uint8_t*)rootfs.block_buf, sizeof(ext4_block));
    ext4_inode* inode = (ext4_inode*)rootfs.block_buf;

    uint64_t root_block = inode[1].i_block[0];
    uint64_t root_dir_start_sector = (FS_START_SECTOR) + (root_block * 8);

    emmc_seek_sector(root_dir_start_sector);
    emmc_read((uint8_t*)rootfs.block_buf, sizeof(ext4_block));

    ext4_dir_entry* dir = (ext4_dir_entry*) rootfs.block_buf;

    INFO("\n\n> ls\n");

    print_dir(dir);

    return root_dir_start_sector;
}

size_t read_dir(size_t sector_num){
    size_t lib_inode = 7340033;

    size_t offset;
    size_t inode_sector = inode_to_sector(lib_inode, &offset);

    emmc_seek_sector(inode_sector);
    emmc_read(rootfs.block_buf, sizeof(ext4_block));

    ext4_inode* inode = (ext4_inode*) UNSCALED_POINTER_ADD(rootfs.block_buf, offset);

    uint32_t dirblock = inode->i_block[0];
    uint32_t dirsector = FS_START_SECTOR + dirblock * 8;

    emmc_seek_sector(dirsector);
    emmc_read(rootfs.block_buf, sizeof(ext4_block));

    INFO("\n\n> ls /lib\n");

    print_dir((ext4_dir_entry*) rootfs.block_buf);
}

void test_read_ls(){
    size_t ls_libnode = 7340034;

    size_t offset;
    size_t inode_sector = inode_to_sector(ls_libnode, &offset);

    emmc_seek_sector(inode_sector);
    emmc_read(rootfs.block_buf, sizeof(ext4_block));
    ext4_inode* inode = (ext4_inode*) UNSCALED_POINTER_ADD(rootfs.block_buf, offset);

    INFO("Inode size: %d\n", inode->i_size_lo);

    // get offset of the first data block
    size_t datasector = FS_START_SECTOR + (inode->i_block[0] * 8);
    emmc_seek_sector(datasector);
    emmc_read(rootfs.block_buf, sizeof(ext4_block));

    INFO("Displaying first 16 bytes of executable...\n");

    for(int i = 0; i < 16; i++){
        kprintf("0x%x ", ((uint8_t*)rootfs.block_buf)[i]);
    }
    kprintf("\n");
}


void filesystem_init(){
    
}
/*
    inode_start_sector = 0x106108
    root_dir_start_sector = 0x10e108

*/

