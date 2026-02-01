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
    return -1;
}