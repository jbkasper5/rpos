#ifndef __LCD_H__
#define __LCD_H__

#define GPU_BUS_TO_ARM(addr) ((addr) & 0x3FFFFFFF)

int init_framebuffer();


typedef struct frame_s{
    uint32_t* fb;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
} frame_t;

#endif