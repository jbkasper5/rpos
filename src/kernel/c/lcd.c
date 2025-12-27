#include "mailbox.h"
#include "lcd.h"
#include "printf.h"
#include "mem.h"

static mailbox_buf_t mb __attribute__((aligned(16)));
static frame_t frame;

static frame_t* _init_framebuffer(mailbox_buf_t* mb, uint32_t width, uint32_t height, uint32_t depth) {
    uint32_t i = 0;

    mb->buffer[i++] = 104;       // Total size in bytes
    mb->buffer[i++] = 0;           // Request code

    // Tag: set physical size
    mb->buffer[i++] = TAG_SET_PHYSICAL_SIZE;
    mb->buffer[i++] = 8;
    mb->buffer[i++] = 0;
    mb->buffer[i++] = width;
    mb->buffer[i++] = height;

    // Tag: set virtual size
    mb->buffer[i++] = TAG_SET_VIRTUAL_SIZE;
    mb->buffer[i++] = 8;
    mb->buffer[i++] = 0;
    mb->buffer[i++] = width;
    mb->buffer[i++] = height;

    // Tag: set depth
    mb->buffer[i++] = TAG_SET_DEPTH;
    mb->buffer[i++] = 4;
    mb->buffer[i++] = 0;
    mb->buffer[i++] = depth;

    // Tag: allocate buffer
    mb->buffer[i++] = TAG_ALLOCATE_BUFFER;
    mb->buffer[i++] = 8;
    mb->buffer[i++] = 0;
    mb->buffer[i++] = 16; // alignment
    mb->buffer[i++] = 0;  // returned framebuffer size

    // Tag: get pitch
    mb->buffer[i++] = TAG_GET_PITCH;
    mb->buffer[i++] = 4;
    mb->buffer[i++] = 0;
    mb->buffer[i++] = 0;

    // End tag
    mb->buffer[i++] = TAG_END;

    // Send request
    mailbox_write((uint64_t)mb & ~0xF, CHANNEL_PROP);

    // Wait for response
    mailbox_read(CHANNEL_PROP);

    uint64_t fb_addr = mb->buffer[19] & 0x3FFFFFFF;  // clear high flags
    // uint32_t fb_size = mb->buffer[20] & 0x3FFFFFFF;
    uint32_t pitch   = mb->buffer[23] & 0x7FFFFFFF;


    // display uses AARRGGBB
    uint32_t* fb_bytes = (uint32_t *)GPU_BUS_TO_ARM(fb_addr);

    frame.fb = fb_bytes;
    frame.width = width;
    frame.height = height;
    frame.pitch = pitch;

    return &frame;
}

static void fill_screen(frame_t* frame, uint32_t argb){
    memset(frame->fb, argb, frame->width * frame->height, sizeof(argb));
}


int init_framebuffer(){
    frame_t* frame = _init_framebuffer(&mb, 800, 480, 32);

    PDEBUG("Frame buffer address: 0x%x\n", frame->fb);

    // argb = 0xFF, 0x00, 0xFF, 0x00 -> green fill
    while(TRUE){
        // green 
        fill_screen(frame, 0xFFde3200);
        fill_screen(frame, 0xFF00c648);
        fill_screen(frame, 0xFF0098ff);
    }
    // PDEBUG("Panel initialization result: %d\n", ok);

    return 0;
}