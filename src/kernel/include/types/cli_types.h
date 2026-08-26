#ifndef __CLI_TYPES_H__
#define __CLI_TYPES_H__

#include "macros.h"

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
    u32 magic;         // 0x864ab572
    u32 version;
    u32 headersize;
    u32 flags;
    u32 glyph_count;
    u32 bytes_per_glyph;
    u32 height;
    u32 width;
} psf2_header_t;

#endif
