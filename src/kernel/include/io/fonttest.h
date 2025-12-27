#ifndef __FONTTEST_H__
#define __FONTTEST_H__

#include "macros.h"

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

void test_font();
void print_glyph(char c);
void print_string(char* s);

#endif