#include "filedescriptors/filedescriptors.h"

int pipe_write(struct file_s* file, const char* buf, uint64_t count){
    pipe_buffer_t* b = (pipe_buffer_t*)file->private_data;

    return 0;
}

int pipe_read(struct file_s* file, char* buf, uint64_t count){

}

int uart_write(struct file_s* file, const char* buf, uint64_t count){
    return kprintf(buf);
}

int uart_read(struct file_s* file, char* buf, uint64_t count){
    
}