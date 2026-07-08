#ifndef __FILESYSTEM_TYPES_H__
#define __FILESYSTEM_TYPES_H__

#include "macros.h"
#include "types/disk_types.h"

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


struct file_s;

typedef struct fileops_s{
    int (*open)(struct file_s* file);
    int (*read)(struct file_s* file, char* buf, u64 count);
    int (*write)(struct file_s* file, const char* buf, u64 count);
    int (*ioctl)(struct file_s* file, unsigned int cmd, unsigned long arg);
    int (*close)(struct file_s* file);
} fileops_t;

typedef struct file_s{
    ext4_inode* inode;        // inode of the file
    u64 pos;                  // current seek position
    u32 flags;                // O_RDONLY, O_RDWR, etc.
    u32 refcount;             // for dup/close
    const fileops_t* file_ops;
    void* private_data;
} file_t;

#endif
