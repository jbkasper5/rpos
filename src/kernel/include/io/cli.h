#ifndef __CLI_H__
#define __CLI_H__

#include "macros.h"
#include "io/lcd.h"

#define NUM_LINES 24
#define LINE_WIDTH 100

typedef enum{
    ANSI_BLACK   = 0xFF000000, // 30
    ANSI_RED     = 0xFFFF0000, // 31
    ANSI_GREEN   = 0xFF00FF00, // 32
    ANSI_YELLOW  = 0xFFFFFF00, // 33
    ANSI_BLUE    = 0xFF0000FF, // 34
    ANSI_MAGENTA = 0xFFFF00FF, // 35
    ANSI_CYAN    = 0xFF00FFFF, // 36
    ANSI_WHITE   = 0xFFFFFFFF, // 37
} ANSI_COLORS;

typedef struct {
    unsigned char magic[2];
    unsigned char mode;
    unsigned char charsize;
} psf1_header_t;

typedef struct {
    uint32_t magic;         // 0x864ab572
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t glyph_count;
    uint32_t bytes_per_glyph;
    uint32_t height;
    uint32_t width;
} psf2_header_t;


extern frame_t frame;

void set_text_background_color();
void set_text_color();

void unset_text_background_color();
void unset_text_color();

void scroll();
void print_glyph(char c);
void print_string(char* s);
void load_font();

#endif