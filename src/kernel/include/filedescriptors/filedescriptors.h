#ifndef __FILE_DESCRIPTORS_H__
#define __FILE_DESCRIPTORS_H__

#include <stdint.h>
#include "filesystem/filesystem.h"
#include "uapi/rpos/fds.h"

int pipe_write(struct file_s* file, const char* buf, uint64_t count);
int pipe_read(struct file_s* file, char* buf, uint64_t count);
int uart_write(struct file_s* file, const char* buf, uint64_t count);
int uart_read(struct file_s* file, char* buf, uint64_t count);

#endif