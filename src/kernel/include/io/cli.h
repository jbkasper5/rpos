#ifndef __CLI_H__
#define __CLI_H__

#include "macros.h"
#include "io/lcd.h"
#include "types/cli_types.h"

#define NUM_LINES 24
#define LINE_WIDTH 100

extern frame_t frame;

void set_text_background_color();
void set_text_color();

void unset_text_background_color();
void unset_text_color();

void fill_screen(frame_t* frame, u32 argb);
void scroll();
void print_glyph(char c);
void print_string(char* s);
void load_font();

#endif