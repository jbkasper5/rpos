#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "macros.h"

bool video_set_resolution(uint32_t xres, uint32_t yres, uint32_t bpp);
void video_draw_char(char c, uint32_t x, uint32_t y);
void video_draw_string(char* str, uint32_t x, uint32_t y);

uint32_t font_get_height();
uint32_t font_get_width();

#endif