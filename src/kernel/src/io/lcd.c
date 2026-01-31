#include "mailbox/mailbox.h"
#include "io/lcd.h"
#include "io/kprintf.h"
#include "memory/mem.h"
#include "macros.h"
#include "memory/mmap.h"

#define NUM_LINES 24
#define LINE_WIDTH 100

frame_t frame;
bool LCD_READY = FALSE;

static uint32_t mb[1024] __attribute__((aligned(16)));

static void _init_framebuffer(uint32_t* mb, uint32_t width, uint32_t height, uint32_t depth) {
    uint32_t i = 0;

    mb[i++] = 104;       // Total size in bytes
    mb[i++] = 0;           // Request code

    // Tag: set physical size
    mb[i++] = TAG_SET_PHYSICAL_SIZE;
    mb[i++] = 8;
    mb[i++] = 0;
    mb[i++] = width;
    mb[i++] = height;

    // Tag: set virtual size
    mb[i++] = TAG_SET_VIRTUAL_SIZE;
    mb[i++] = 8;
    mb[i++] = 0;
    mb[i++] = width;
    mb[i++] = height;

    // Tag: set depth
    mb[i++] = TAG_SET_DEPTH;
    mb[i++] = 4;
    mb[i++] = 0;
    mb[i++] = depth;

    // Tag: allocate buffer
    mb[i++] = TAG_ALLOCATE_BUFFER;
    mb[i++] = 8;
    mb[i++] = 0;
    mb[i++] = 16; // alignment
    mb[i++] = 0;  // returned framebuffer size

    // Tag: get pitch
    mb[i++] = TAG_GET_PITCH;
    mb[i++] = 4;
    mb[i++] = 0;
    mb[i++] = 0;

    // End tag
    mb[i++] = TAG_END;

    // Send request
    mailbox_write(CHANNEL_PROP, (uint64_t)mb & ~0xF);

    // Wait for response
    mailbox_read(CHANNEL_PROP);

    uint64_t fb_addr = mb[19] & 0x3FFFFFFF;  // clear high flags
    // uint32_t fb_size = mb[20] & 0x3FFFFFFF;
    uint32_t pitch   = mb[23] & 0x7FFFFFFF;


    // display uses AARRGGBB
    uint32_t* fb_bytes = (uint32_t *)GPU_BUS_TO_ARM(fb_addr);

    frame.fb = fb_bytes;
    frame.width = width;
    frame.height = height;
    frame.pitch = pitch;
}

void write_pixel(uint64_t idx, uint32_t argb){
    frame.fb[idx] = argb;
}

int panel_ready(){
    return LCD_READY;
}

int init_framebuffer(){
    _init_framebuffer(mb, 800, 480, 32);
    DEBUG("Frame buffer address: 0x%x\n", frame.fb);
    LCD_READY = TRUE;
    return 0;
}