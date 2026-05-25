#ifndef __FILE_DESCRIPTORS_H__
#define __FILE_DESCRIPTORS_H__

#include <stdint.h>
#include "filesystem/filesystem.h"
#include "uapi/rpos/fds.h"
#include "filedescriptors/pipe.h"

int pipe_write(struct file_s* file, const char* buf, u64 count);
int pipe_read(struct file_s* file, char* buf, u64 count);
int pipe_open(struct file_s* file);
int uart_write(struct file_s* file, const char* buf, u64 count);
int uart_read(struct file_s* file, char* buf, u64 count);

const fileops_t stdout_ops = {
    .write = uart_write,
    .read = uart_read,
    .close = NULL,
    .ioctl = NULL,
};

const fileops_t pipe_ops = {
    .write = pipe_write,
    .read = pipe_read,
    .close = pipe_open,
    .ioctl = NULL
};

const file_t stdio_file = {
    .file_ops = &stdout_ops,
    .flags = 0,
    .inode = NULL,
    .pos = 0,
    .private_data = NULL,
    .refcount = 1
};

const file_t pipe_file = {
    .file_ops = &stdout_ops,
    .flags = 0,
    .inode = NULL,
    .pos = 0,
    .private_data = NULL,
    .refcount = 1
};

#endif