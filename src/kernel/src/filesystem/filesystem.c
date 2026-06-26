#include "filesystem/filesystem.h"
#include "memory/kmalloc.h"
#include "memory/mem.h"
#include "utils/datastructures.h"
#include "utils/utils.h"
#include "uapi/rpos/errno.h"

#define DELIMITER   '/'
#define MAX_NAME    255

extern trie* device_trie;

u64 check_vfs(char* path){
    return trie_get(device_trie, path);
}

void* open(const char* pathname, u32 flags){
    INFO("Opening path '%s'\n", pathname);

    // make a mutable copy of the path
    u32 len = strlen(pathname);
    char* str = (char*) kmalloc(len + 1);
    if(!str){
        ERROR("OUT OF MEMORY.\n");
        return NULL;
    }
    memcpy(str, pathname, len);
    str[len] = '\0';

    char* savestr;
    char* token = strtok(str, '/', &savestr);

    ext4_inode* nodeptr = kmalloc(sizeof(ext4_inode));
    
    if(pathname[0] == '/'){
        memcpy(nodeptr, rootfs.root_inode, sizeof(ext4_inode));
    }else{
        return NULL;
    }

    while(token){
        nodeptr = lookup(nodeptr, token);
        if(!nodeptr){
            WARNING("FILE NOT FOUND. Could not resolve '%s' in '%s'.\n", token, pathname);
            return NULL;
        }
        token = strtok(NULL, '/', &savestr);
    }

    kfree(str);

    if(nodeptr){
        INFO("'%s' resolved successfully.\n", pathname);
        file_t* file = (file_t*) kmalloc(sizeof(file_t));
        file->file_ops = NULL;
        file->inode = nodeptr;
        file->pos = 0;
        file->flags = 0;
        kfree(nodeptr);
        return file;
    }

    WARNING("Could not resolve path '%s'\n", pathname);
    kfree(nodeptr);
    return NULL;
}


int close(file_t* file){
    if(file){
        DEBUG("Closing file pointer inode...\n");
        kfree(file->inode);

        DEBUG("Closing file pointer...\n");
        kfree(file);

        DEBUG("File closed.\n");
        return 0;
    }
    return -1;
}


static u32 read_file_block(file_t* file, void* buf, u64 count, u32 blockno){
    u32 block_offset = file->pos % 4096;
    u32 bytes_to_read = MIN(count, 4096 - block_offset);

    // if the block is empty, just fill it with 0s
    if(!blockno){
        memset((u8*) buf, 0, bytes_to_read);
        file->pos += bytes_to_read;
        return bytes_to_read;
    }

    // read the block into the scratch buffer
    read_block(rootfs.block_buf, blockno);

    // advance the pointer to the offset in the block
    u8* ptr = UNSCALED_POINTER_ADD(rootfs.block_buf->data, block_offset);

    // copy the data from the block into the buffer
    memcpy((u8*) buf, ptr, bytes_to_read);

    // advance the file position 
    file->pos += bytes_to_read;

    // return number of bytes read
    return bytes_to_read;
}

static u64 read_direct(file_t* file, void* buf, u64 count){
    u32 block_no = file->pos / 4096;
    u32 block_offset = file->pos % 4096;

    u64 bytes_read = 0;
    u64 bytes_to_read = count;
    for(int i = block_no; i < 15; i++){
        void* buf_advanced = UNSCALED_POINTER_ADD(buf, bytes_read);
        bytes_read += read_file_block(file, buf_advanced, count - bytes_read, file->inode->i_block[i]);

        block_offset = 0; // only offset for the first block

        if(bytes_read >= count){
            break;
        }
    }

    return bytes_read;
}

static u64 read_single_indirect(file_t* file, void* buf, u64 count, u64 offset){
    // get the indirect block index from the file pointer
    u64 single_indirect_blockno = file->inode->i_block[12];

    // load in the list of direct blocks
    u32* blocks = (u32*) kmalloc(sizeof(ext4_block));
    read_block(blocks, single_indirect_blockno);

    // get the proper start direct block
    u32 new_blockno = blocks[offset];


    u64 bytes_read = 0;
    while(bytes_read < count){
        
        // auto-advances file->pos
        void* buf_advanced = UNSCALED_POINTER_ADD(buf, bytes_read);

        bytes_read += read_file_block(file, buf_advanced, count - bytes_read, new_blockno);

        // move to the next indirect block number
        new_blockno = blocks[++offset];

        // there are 128 possible entries in a single indirect block, so if we 
        // exceed it, then we advance into the double indirect space
        if(new_blockno >= 128) break;
    }

    // free block buffer
    kfree(blocks);

    return bytes_read;
}

static u64 read_double_indirect(file_t* file, void* buf, u64 count, u64 offset){
    u64 double_indirect_blockno = file->inode->i_block[13];
    u32* blocks = (u32) rootfs.block_buf;
    read_block(blocks, double_indirect_blockno);
}

static u64 read_triple_indirect(file_t* file, void* buf, u64 count, u64 offset){
    
}


u64 read(file_t* file, void* buf, u64 count){
    if(!file || !buf){
        return -1;
    }

    u64 entries_per_block = (1 << 10);
    u64 entries_per_double_indirect = entries_per_block * entries_per_block;
    u64 entries_per_triple_indirect = entries_per_double_indirect* entries_per_block;

    u32 blockno = file->pos / (1 << 12);
    if (blockno < 12) {
        return read_direct(file, buf, count);
    }
    else if (blockno < 12 + entries_per_block) {
        return read_single_indirect(file, buf, count, blockno - 12);
    }
    else if (blockno < 12 + entries_per_block + entries_per_double_indirect) {
        return read_double_indirect(file, buf, count, blockno - (12 + entries_per_block));
    }
    else if (blockno < 12 + entries_per_block + entries_per_double_indirect + entries_per_triple_indirect) {
        return read_triple_indirect(file, buf, count, blockno - (12 + entries_per_block + entries_per_double_indirect));
    }

    ERROR("Block number exceeds EXT filesystem limits\n");
    return -1;
}


int seek(file_t* file, u64 offset, int whence){
    if(whence == SEEK_SET){
        file->pos = offset;
    }else if(whence == SEEK_CUR){
        file->pos += offset;
    }else if(whence == SEEK_END){
        ERROR("SEEK_END is not currently supported.\n");
        return -1;
    }
    return 0;
}

//         0xffff80003ffec080
// second: 0xffff80003ffec080