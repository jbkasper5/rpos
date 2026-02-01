#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "macros.h"
#include "filesystem/disk.h"

typedef struct {
    ext4_inode* inode;      // inode of the file
    uint64_t pos;            // current seek position
    uint32_t flags;          // O_RDONLY, O_RDWR, etc.
    uint32_t refcount;       // for dup/close
} file_t;

int open(const char* pathname, uint32_t flags);

#endif