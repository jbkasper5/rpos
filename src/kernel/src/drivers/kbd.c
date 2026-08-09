#include "drivers/kbd.h"
#include "filedescriptors/pipe.h"
#include "memory/kmalloc.h"
#include "filesystem/filesystem.h"
#include "system/process.h"

extern u8 kbd_flag;
u8* flag = &kbd_flag;

static pipe_data_t kbd_pipe = {
    .buf = NULL,
    .head = NULL,
    .tail = NULL,
    .rd_wait = NULL,
    .wr_wait = NULL,
    .readers = 0,
    .writers = 0,
    .ring_size = 0
};


// called during boot to start handling keypresses
void keyboard_init(){
    // register a flag with the IRQ to start accepting redirects to handle_keyboard_event
    kbd_pipe.buf = (u8*) kmalloc(PAGE_SIZE);

    kbd_pipe.head = (u32) kbd_pipe.buf;
    kbd_pipe.tail = (u32) kbd_pipe.buf;
    kbd_pipe.ring_size = PAGE_SIZE;

    // initialize the wait lists
    INIT_LIST_HEAD(&kbd_pipe.rd_wait.head);
    INIT_LIST_HEAD(&kbd_pipe.wr_wait.head);

    // register the keyboard as active with the IRQ handler
    *flag = 1;
}

int keyboard_open(struct file_s* file){
    // open the pipe buffer that the keyboard will stream to
    file->private_data = &kbd_pipe;
    file->refcount++;
    return TRUE;
}

int keyboard_close(struct file_s* file){
    return -1;
}

static void wake_listeners(pipe_data_t *p){
    // spinlock acquire
    while(!list_empty(&p->rd_wait.head)){
        wait_queue_entry_t* entry = list_entry(p->rd_wait.head.next, wait_queue_entry_t, entry);

        // call the wait function from the entry (in this case, the wake_kbd function)
        entry->func(entry);

        // remove the waiter from the queue
        list_remove(&entry->entry);
    }
}


void handle_keyboard_event(char c) {
    /* occupancy = head - tail; full when it equals the buffer size */
    if (kbd_pipe.head - kbd_pipe.tail >= kbd_pipe.ring_size) return;

    u32 loc = kbd_pipe.head % kbd_pipe.ring_size;
    kbd_pipe.buf[loc] = (u8)c;

    __asm__ volatile("dmb ish" ::: "memory");    /* see note below */
    kbd_pipe.head++;

    uart_putc(c);

    wake_listeners(&kbd_pipe);
}