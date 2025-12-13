#include "mailbox.h"
#include "lcd.h"
#include "printf.h"
#include "mem.h"

#define NUM_LINES 24
#define LINE_WIDTH 100

bool LCD_READY = FALSE;

static mailbox_buf_t mb __attribute__((aligned(16)));
static frame_t* frame;

static void _init_framebuffer(mailbox_buf_t* mb, uint32_t width, uint32_t height, uint32_t depth) {
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

    frame->fb = fb_bytes;
    frame->width = width;
    frame->height = height;
    frame->pitch = pitch;
}

static void fill_screen(frame_t* frame, uint32_t argb){
    // frame width = 800
    // frame hieght = 480
    // sizeof(argb) = 4
    memset(frame->fb, argb, frame->width * frame->height, sizeof(argb));
}

void write_pixel(uint64_t idx, uint32_t argb){
    frame->fb[idx] = argb;
}

void scroll(){
    // get number of consecutive bytes that make up one line on the screen
    // 800 pixels * size per pixel for 1 row
    // multiply by 20 since thats the height of 1 glyph
    uint64_t line_bytes = 800 * 20;

    memcpy(frame->fb, frame->fb + line_bytes, line_bytes * 16 * (NUM_LINES - 1));

    uint32_t* last_line = frame->fb + (line_bytes * (NUM_LINES - 1));

    memset(last_line, 0xFFFFFFFF, line_bytes, sizeof(uint32_t));
}

int panel_ready(){
    return LCD_READY;
}

int init_framebuffer(){
    _init_framebuffer(&mb, 800, 480, 32);

    PDEBUG("Frame buffer address: 0x%x\n", frame->fb);

    // green fill
    // fill_screen(frame, 0xFF00c648);

    fill_screen(frame, 0xFFFFFFFF);
    // PDEBUG("Panel initialization result: %d\n", ok);

    LCD_READY = TRUE;
    return 0;
}