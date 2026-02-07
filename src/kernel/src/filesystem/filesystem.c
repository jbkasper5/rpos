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


uint64_t read(file_t* file, void* buf, uint64_t count){
    uint64_t total_bytes_read = 0;
    uint8_t* buffer = (uint8_t*) buf;
    uint64_t bytes_to_read = 0;

    for(int i = 0; i < 15; i ++){
        // look for a valid block to read
        if(!file->inode->i_block[i]) continue;

        // once a block is found, read it into the block scratch buffer
        read_block(rootfs.block_buf, file->inode->i_block[i]);

        // determine how many bytes to read from this block
        bytes_to_read = MIN(count - total_bytes_read, 4096);

        // copy the data from the block buffer to the user buffer
        memcpy(buffer + total_bytes_read, rootfs.block_buf, bytes_to_read);

        // if we read the right number of bytes, exit
        total_bytes_read += bytes_to_read;

        // advance the position of the filepointer
        file->pos += bytes_to_read;

        // if we have read enough bytes, break
        if(total_bytes_read >= count) break;
    }

    return total_bytes_read;
}