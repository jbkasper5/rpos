#include "filedescriptors/filedescriptors.h"
#include "memory/kmalloc.h"

int pipe_write(struct file_s* file, const char* buf, u64 count){
    pipe_buffer_t* b = (pipe_buffer_t*)file->private_data;

    return 0;
}

int pipe_read(struct file_s* file, char* buf, u64 count){
    // use file->pos to read from the pipe
    u32 bytes_to_read = MIN(count, PAGE_SIZE - file->pos);
    memcpy(buf, file->private_data, bytes_to_read);
    return bytes_to_read;
}

int pipe_open(struct file_s* file){
    // allocate the pipe's buffer (for now 1 page)
    file->private_data = kmalloc(sizeof(pipe_data_t));

    if(!file->private_data) return FALSE;

    // allocate pipe data and store it in the private data handle
    pipe_data_t* priv_data = (pipe_data_t*) file->private_data;

    // allocate buffer metadata for the pipe
    priv_data->bufs = kmalloc(sizeof(pipe_buffer_t));

    if(!priv_data->bufs){
        kfree(priv_data);
        return FALSE;
    }

    // allocate a page buffer for the pipe
    priv_data->bufs->page = kmalloc(PAGE_SIZE);

    if(!priv_data->bufs->page){
        kfree(priv_data);
        kfree(priv_data->bufs);
        return FALSE;
    }

    priv_data->head = priv_data->bufs->page;
    priv_data->tail = priv_data->bufs->page;

    return TRUE;
}

int uart_write(struct file_s* file, const char* buf, u64 count){
    kprintf(buf);
    return 0;
}

int uart_read(struct file_s* file, char* buf, u64 count){
    
}