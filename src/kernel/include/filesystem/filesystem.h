#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "macros.h"
#include "filesystem/disk.h"
#include "types/filesystem_types.h"

int close(file_t* file);
void* open(const char* pathname, u32 flags);
u64 read(file_t* file, void* buf, u64 count);
int seek(file_t* file, u64 offset, int whence);

u64 check_vfs(char* path);

#endif
