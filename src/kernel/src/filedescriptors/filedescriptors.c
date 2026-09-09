#include "filedescriptors/filedescriptors.h"
#include "memory/kmalloc.h"
#include "io/gpio.h"
#include "peripherals/bsc.h"
#include "uabi/rpos/ioctls.h"

/*––––––––––––––––––––––––––––– PIPE FUNCTIONS –––––––––––––––––––––––––––––*/
/// @brief 
/// @param file 
/// @param buf 
/// @param count 
/// @return 
int pipe_write(struct file_s* file, const char* buf, u64 count){
    pipe_data_t* pipe_data = (pipe_data_t*) file->private_data;
    if(!pipe_data) return 0;

    // determine the most amount of data we can write to the pipe
    u64 bytes_to_write = MIN(count, pipe_data->ring_size);

    return 0;
}

/// @brief 
/// @param file 
/// @param buf 
/// @param count 
/// @return 
int pipe_read(struct file_s* file, char* buf, u64 count){
    // use file->pos to read from the pipe
    pipe_data_t* pipe_data = (pipe_data_t*) file->private_data;
    
    DEFINE_WAIT(waitqueue_entry);

    // buffer is empty, nothing to read
    while (pipe_data->head == pipe_data->tail){
        if(list_empty(&pipe_data->rd_wait.head)){
            list_add(&waitqueue_entry.entry, &pipe_data->rd_wait.head);
        }
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

/// @brief 
/// @param file 
/// @return 
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

/// @brief 
/// @param file 
/// @return 
int pipe_close(struct file_s* file){
    pipe_data_t* priv_data = (pipe_data_t*) file->private_data;

    // close buffers used by the pipe
    // TODO: needs to handle more than 1 access to the pipe
    kfree(priv_data->buf);
    kfree(file->private_data);

    return TRUE;
}
/*––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/


/*––––––––––––––––––––––––––––– UART FUNCTIONS –––––––––––––––––––––––––––––*/
/// @brief 
/// @param file 
/// @param buf 
/// @param count 
/// @return 
int uart_write(struct file_s* file, const char* buf, u64 count){
    kprintf(buf);
    return 0;
}

/// @brief 
/// @param file 
/// @param buf 
/// @param count 
/// @return 
int uart_read(struct file_s* file, char* buf, u64 count){
    
}
/*––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/

/*––––––––––––––––––––––––––––– I2C FUNCTIONS –––––––––––––––––––––––––––––*/
int i2c_open(file_t* fd){
    #define SDA_GPIO    2
    #define SCL_GPIO    3

    INFO("Opening i2c file...\n");

    // enable the I2C pins and set them to Alt0
    gpio_pin_enable(SDA_GPIO);
    gpio_pin_enable(SCL_GPIO);
    gpio_pin_set_func(SDA_GPIO, GFAlt0);
    gpio_pin_set_func(SCL_GPIO, GFAlt0);

    REGS_BSC1->c.bits.i2cen = 1;
    REGS_BSC1->div.bits.cdiv = 1500;

    INFO("GPIO confiured.\n");
}

int i2c_read(file_t* fd, const char* buf, u64 count){
    if(!buf){
        WARNING("Attempted I2C read on NULL buffer address\n");
        return 0;
    }
    INFO("Reading i2c file...\n");

    REGS_BSC1->a.bits.addr = (u8)(uintptr_t) fd->private_data;
}

static u32 transfer_block(char* buf, u64 count){
    u64 written = 0;

    // start the transfer
    REGS_BSC1->c.bits.st = 1;

    while(written < count){

        // stall if the queue is full
        while(!REGS_BSC1->s.bits.txd);

        DEBUG("tx %x\n", buf[written]);

        // write the next byte once there's room in the FIFO queue
        REGS_BSC1->fifo.bits.data = buf[written++];
    }

    // wait for the transfer to complete
    while (!REGS_BSC1->s.bits.done);

    INFO("Transfer complete status: 0x%x\n", REGS_BSC1->s.bits.err);

    return REGS_BSC1->s.bits.err;
}

int i2c_write(file_t* fd, char* buf, u64 count){
    if(!buf){
        WARNING("Attempted I2C write on NULL buffer address\n");
        return 0;
    }

    u32 dlen_max = (1 << 16) - 1;

    if(count > dlen_max){
        WARNING("Attempted I2C message longer than supported message width.\n");
        return 0;
    }

    // specify slave device address
    REGS_BSC1->a.bits.addr = (u8)(uintptr_t) fd->private_data;

    // clear the transfer buffer
    REGS_BSC1->c.bits.clear = 1;

    // set direction as write
    REGS_BSC1->c.bits.read = 0;

    // set clock frequency 
    REGS_BSC1->div.bits.cdiv = 2500;

    // Transfer can support a maximum of 1 << 15 bytes, and the FIFO is a 16-byte queue
    // transfer needs to continously top off the FIFO while there are still bytes to write
    u64 to_transfer = MIN(count, dlen_max);
    while(to_transfer){
        REGS_BSC1->dlen.bits.dlen = to_transfer;
        u32 status = transfer_block(buf, to_transfer);
        // check for err
        to_transfer = MIN(count - to_transfer, dlen_max);
    }
}

int i2c_ioctl(file_t* fd, u64 cmd, u64 arg){
    INFO("IOCTL request for i2c file...\n");
    switch(cmd){
        case I2C_SLAVE:
            fd->private_data = (void*) arg;   // stash the 7-bit addr
            return 0;
    }
    return -1;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/

// 0xffff80003ffcadf0