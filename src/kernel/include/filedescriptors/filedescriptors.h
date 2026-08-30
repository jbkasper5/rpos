#ifndef __FILE_DESCRIPTORS_H__
#define __FILE_DESCRIPTORS_H__

#include <stdint.h>
#include "filesystem/filesystem.h"
#include "uabi/rpos/fds.h"
#include "filedescriptors/pipe.h"

int pipe_write(struct file_s* file, const char* buf, u64 count);
int pipe_read(struct file_s* file, char* buf, u64 count);
int pipe_open(struct file_s* file);
int pipe_close(struct file_s* file);

int i2c_open(file_t* fd);
int i2c_read(file_t* fd, const char* buf, u64 count);
int i2c_write(file_t* fd, char* buf, u64 count);
int i2c_ioctl(file_t* fd, u64 cmd, u64 arg);

int keyboard_open(struct file_s* file);
int keyboard_close(struct file_s* file);

int uart_write(struct file_s* file, const char* buf, u64 count);
int uart_read(struct file_s* file, char* buf, u64 count);

static const fileops_t stdout_ops = {
    .write = uart_write,
    .read  = uart_read,
    .close = NULL,
    .ioctl = NULL,
};

static const fileops_t pipe_ops = {
    .write = pipe_write,
    .read  = pipe_read,
    .open  = pipe_open,
    .ioctl = NULL,
};

static const fileops_t keyboard_ops = {
    .open  = keyboard_open,        /* this is what does the extra work */
    .read  = pipe_read,       /* generic pipe drainage */
    .write = NULL,            /* keyboards don't accept writes from userspace */
    .close = keyboard_close,
    .ioctl = NULL,
};

static const fileops_t i2c_ops = {
    .open  = i2c_open,
    .read  = i2c_read,
    .write = i2c_write,
    .close = NULL,
    .ioctl = i2c_ioctl,
};

static const file_t stdio_file = {
    .file_ops     = &stdout_ops,
    .flags        = 0,
    .inode        = NULL,
    .pos          = 0,
    .private_data = NULL,
    .refcount     = 1,
};

static const file_t pipe_file = {
    .file_ops     = &pipe_ops,
    .flags        = 0,
    .inode        = NULL,
    .pos          = 0,
    .private_data = NULL,
    .refcount     = 1,
};

static const file_t keyboard_file = {
    .file_ops     = &keyboard_ops,
    .flags        = 0,
    .inode        = NULL,
    .pos          = 0,
    .private_data = NULL,
    .refcount     = 1,
};

static const file_t i2c_file = {
    .file_ops     = &i2c_ops,
    .flags        = 0,
    .inode        = NULL,
    .pos          = 0,
    .private_data = NULL,
    .refcount     = 1,
};

#endif