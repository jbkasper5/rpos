#include "filedescriptors/filedescriptors.h"
#include "memory/kmalloc.h"

int pipe_write(struct file_s* file, const char* buf, u64 count){
    pipe_data_t* pipe_data = (pipe_data_t*) file->private_data;
    if(!pipe_data) return 0;

    // determine the most amount of data we can write to the pipe
    u64 bytes_to_write = MIN(count, pipe_data->ring_size);

    return 0;
}

int pipe_read(struct file_s* file, char* buf, u64 count){
    // use file->pos to read from the pipe
    pipe_data_t* pipe_data = (pipe_data_t*) file->private_data;

    // buffer is empty, nothing to read
    while (pipe_data->head == pipe_data->tail){
        // deschedule current process, for blocking reads
        deschedule();
    }

    /* How much is available, capped by what the caller asked for. */
    u32 avail   = pipe_data->head - pipe_data->tail;
    u32 to_read = MIN(avail, (u32)count);

    /* Where the reader currently sits in the physical buffer. */
    u32 tail_idx = pipe_data->tail & (pipe_data->ring_size - 1);   /* ring_size is a power of 2 */

    /* Bytes from tail_idx to the end of the array, before wrapping. */
    u32 first_chunk = MIN(to_read, pipe_data->ring_size - tail_idx);

    __asm__ volatile("dmb ish" ::: "memory");

    // copy the remainder of the physical buffer
    memcpy(buf, pipe_data->buf + (pipe_data->tail % pipe_data->ring_size), first_chunk);

    u32 second_chunk = to_read - first_chunk;
    if (second_chunk > 0) memcpy(buf + first_chunk, pipe_data->buf, second_chunk);

    pipe_data->tail += to_read;             /* publish: this space is now free for the writer */

    return to_read;
}

int pipe_open(struct file_s* file){
    // allocate the pipe's buffer (for now 1 page)
    file->private_data = kmalloc(sizeof(pipe_data_t));

    if(!file->private_data) return FALSE;

    // allocate pipe data and store it in the private data handle
    pipe_data_t* priv_data = (pipe_data_t*) file->private_data;

    // allocate buffer metadata for the pipe
    priv_data->buf = kmalloc(PAGE_SIZE);

    if(!priv_data->buf){
        kfree(priv_data);
        return FALSE;
    }

    priv_data->head = priv_data->buf;
    priv_data->tail = priv_data->buf;

    priv_data->ring_size = PAGE_SIZE;

    return TRUE;
}


int pipe_close(struct file_s* file){
    pipe_data_t* priv_data = (pipe_data_t*) file->private_data;

    // close buffers used by the pipe
    // TODO: needs to handle more than 1 access to the pipe
    kfree(priv_data->buf);
    kfree(file->private_data);

    return TRUE;
}

int uart_write(struct file_s* file, const char* buf, u64 count){
    kprintf(buf);
    return 0;
}

int uart_read(struct file_s* file, char* buf, u64 count){
    
}