#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "macros.h"
#include "filesystem/disk.h"
#include "filedescriptors/filedescriptors.h"

typedef struct {
	// struct page *page;
    void* page;
	unsigned int offset, len;
	// const struct pipe_buf_operations *ops;
	unsigned int flags;
	unsigned long private;
} pipe_buffer_t;


typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1,
    SEEK_END = 2
} seek_whence;


typedef struct fileops_s{
    int (*read)(struct file_s* file, char* buf, u64 count);
    int (*write)(struct file_s* file, const char* buf, u64 count);
    int (*ioctl)(struct file_s* file, unsigned int cmd, unsigned long arg);
    int (*close)(struct file_s* file);
} fileops_t;

typedef struct file_s{
    ext4_inode* inode;      // inode of the file
    u64 pos;            // current seek position
    u32 flags;          // O_RDONLY, O_RDWR, etc.
    u32 refcount;       // for dup/close
    fileops_t* file_ops;
    void* private_data;
} file_t;

int close(file_t* file);
void* open(const char* pathname, u32 flags);
u64 read(file_t* file, void* buf, u64 count);
int seek(file_t* file, u64 offset, int whence);

u8 check_vfs(char* path);

#endif