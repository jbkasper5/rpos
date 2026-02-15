#include "filesystem/filesystem.h"
#include "memory/kmalloc.h"
#include "memory/mem.h"

#define DELIMITER   '/'
#define MAX_NAME    255

static ext4_inode* resolve_path(const char* pathname){
    char* buf = (char*) kmalloc(MAX_NAME);
    int start = 0, i = 0;

    while(pathname[i]){
        if(pathname[i] == DELIMITER){
            if(start == 0){
                start = i + 1; 
                i++;
                continue;
            }
            memcpy(buf, pathname + start, i - start);
            buf[i - start] = '\0';

            // skip past delimiter
            start = i + 1;

            INFO("Parsed segment: '%s'\n", buf);
        }else{

        }
        i++;
    }

    if(i > start && i > 0){
        memcpy(buf, pathname + start, i - start);
        buf[i - start] = '\0';
        INFO("Parsed segment: '%s'\n", buf);
    }

    kfree(buf);
}

int open(const char* pathname, uint32_t flags){
    INFO("Opening path '%s'\n", pathname);
    ext4_inode* inode = resolve_path(pathname);


    // for now, pathname = 'bin/ls'
    ext4_inode* dirnode = lookup(rootfs.root_inode, "bin");
    if(dirnode != NULL){
        INFO("Found 'bin' directory inode.\n");
        ext4_inode* filenode = lookup(dirnode, "ls");
        if(filenode){
            INFO("Found 'ls' file inode.\n");
            kfree(dirnode);
            file_t* file = (file_t*) kmalloc(sizeof(file_t));
            if(!file){
                ERROR("Failed to allocate memory for file structure.\n");
                return NULL;
            }
            file->inode = filenode;
            file->pos = 0;
            file->flags = 0;
            return file;
        }else{
            ERROR("File 'ls' not found in 'bin' directory.\n");
        }
    }
    kfree(dirnode);
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


static uint32_t read_file_block(file_t* file, void* buf, uint64_t count, uint32_t blockno){
    uint32_t block_offset = file->pos % 4096;
    uint32_t bytes_to_read = MIN(count, 4096 - block_offset);

    // if the block is empty, just fill it with 0s
    if(!blockno){
        memset((uint8_t*) buf, 0, bytes_to_read);
        file->pos += bytes_to_read;
        return bytes_to_read;
    }

    // read the block into the scratch buffer
    read_block(rootfs.block_buf, blockno);

    // advance the pointer to the offset in the block
    uint8_t* ptr = UNSCALED_POINTER_ADD(rootfs.block_buf->data, block_offset);

    // copy the data from the block into the buffer
    memcpy((uint8_t*) buf, ptr, bytes_to_read);

    // advance the file position 
    file->pos += bytes_to_read;

    // return number of bytes read
    return bytes_to_read;
}


static uint64_t read_direct(file_t* file, void* buf, uint64_t count){
    uint32_t block_no = file->pos / 4096;
    uint32_t block_offset = file->pos % 4096;

    uint64_t bytes_read = 0;
    uint64_t bytes_to_read = count;
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

static uint64_t read_single_indirect(file_t* file, void* buf, uint64_t count, uint64_t offset){
    // get the indirect block index from the file pointer
    uint64_t single_indirect_blockno = file->inode->i_block[12];

    // load in the list of direct blocks
    uint32_t* blocks = (uint32_t*) kmalloc(sizeof(ext4_block));
    read_block(blocks, single_indirect_blockno);

    // get the proper start direct block
    uint32_t new_blockno = blocks[offset];


    uint64_t bytes_read = 0;
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

static uint64_t read_double_indirect(file_t* file, void* buf, uint64_t count, uint64_t offset){
    uint64_t double_indirect_blockno = file->inode->i_block[13];
    uint32_t* blocks = (uint32_t) rootfs.block_buf;
    read_block(blocks, double_indirect_blockno);
}

static uint64_t read_triple_indirect(file_t* file, void* buf, uint64_t count, uint64_t offset){
    
}



uint64_t read(file_t* file, void* buf, uint64_t count){
    if(!file || !buf){
        return -1;
    }

    uint64_t entries_per_block = (1 << 10);
    uint64_t entries_per_double_indirect = entries_per_block * entries_per_block;
    uint64_t entries_per_triple_indirect = entries_per_double_indirect* entries_per_block;

    uint32_t blockno = file->pos / (1 << 12);
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


int seek(file_t* file, uint64_t offset, int whence){
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