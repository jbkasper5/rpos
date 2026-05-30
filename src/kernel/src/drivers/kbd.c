#include "drivers/kbd.h"
#include "filedescriptors/pipe.h"
#include "memory/kmalloc.h"
#include "filesystem/filesystem.h"

extern u8 kbd_flag;
u8* flag = &kbd_flag;

static pipe_data_t kbd_pipe = {
    .buf = NULL,
    .head = NULL,
    .tail = NULL,
    .readers = 0,
    .writers = 0,
    .ring_size = 0
};


// called during boot to start handling keypresses
void keyboard_init(){
    // register a flag with the IRQ to start accepting redirects to handle_keyboard_event
    kbd_pipe.buf = (u8*) kmalloc(PAGE_SIZE);

    kbd_pipe.head = kbd_pipe.buf;
    kbd_pipe.tail = kbd_pipe.buf;
    kbd_pipe.ring_size = PAGE_SIZE;

    // register the keyboard as active with the IRQ handler
    *flag = 1;
}

int keyboard_open(struct file_s* file){
    // open the pipe buffer that the keyboard will stream to
    file->private_data = &kbd_pipe;
    file->refcount++;
}

int keyboard_close(struct file_s* file){
    return -1;
}


void handle_keyboard_event(char c) {
    /* occupancy = head - tail; full when it equals the buffer size */
    if (kbd_pipe.head - kbd_pipe.tail >= kbd_pipe.ring_size) return;

    u32 loc = kbd_pipe.head % kbd_pipe.ring_size;
    kbd_pipe.buf[loc] = (u8)c;

    __asm__ volatile("dmb ish" ::: "memory");    /* see note below */
    kbd_pipe.head++;
}