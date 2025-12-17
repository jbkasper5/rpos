#ifndef __LCD_H__
#define __LCD_H__

#include "macros.h"

#define GPU_BUS_TO_ARM(addr) ((addr) & 0x3FFFFFFF)

typedef struct frame_s{
    uint32_t* fb;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
} frame_t;

void scroll();
int panel_ready();
int init_framebuffer();
void write_pixel(uint64_t idx, uint32_t argb);

#endif