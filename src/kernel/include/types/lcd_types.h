#ifndef __LCD_TYPES_H__
#define __LCD_TYPES_H__

#include "macros.h"

typedef struct frame_s{
    u32* fb;
    u32 width;
    u32 height;
    u32 pitch;
} frame_t;

#endif
