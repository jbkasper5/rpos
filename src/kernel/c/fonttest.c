#include "macros.h"
#include "printf.h"
#include "fonttest.h"
#include "lcd.h"

#define EMPTY_GLYPH ' '

extern unsigned char _binary_src_kernel_fonts_tamzen10x20_psf_end[];
extern unsigned char _binary_src_kernel_fonts_tamzen10x20_psf_size[];
extern unsigned char _binary_src_kernel_fonts_tamzen10x20_psf_start[];

unsigned char* font = _binary_src_kernel_fonts_tamzen10x20_psf_start;
unsigned long font_size = (unsigned long)_binary_src_kernel_fonts_tamzen10x20_psf_size;
uint64_t line = 0;
uint64_t cursor = 0;

static void newline(){
    line += 1;
    cursor = 0;

    // if we are about to overflow the screen, slide everything up
    if (line == 24){
        // need to memcpy everything after the first line to framebuffer 0
        scroll();
        line--;
    }
}

static void _print_glyph(unsigned char* glyph_location){
    int fb_idx = (line * 800 * 20) + cursor;
    uint32_t color;

    int bytes_per_row = 2;  // 10 pixels → 2 bytes
    for (int row = 0; row < 20; row++){
        // Read the 2 bytes of this row
        uint16_t r = (glyph_location[row * bytes_per_row] << 8) | 
                      glyph_location[row * bytes_per_row + 1];

        uint16_t mask = 1 << 15;  // start at leftmost bit (MSB = pixel 0)
        for (int col = 0; col < 10; col++){
            if (r & mask) color = 0xFF000000;  // black pixel
            else          color = 0xFFFFFFFF;  // white pixel

            write_pixel(fb_idx + (row * 800) + col, color);
            mask >>= 1;
        }
    }
    cursor += 10;

    // newline
    if(cursor == 800){
        newline();
    }
}

void print_glyph(char c){
    if(c == '\n' || c == '\r'){
        newline();
    }else if(c == 127){
        // delete key
        if(cursor > 0){
            cursor -= 10;

            // overwrite previous glyph
            _print_glyph(UNSCALED_POINTER_ADD(font, (40 * EMPTY_GLYPH)));

            // move back 1 again
            cursor -= 10;
        }
    }else{
        _print_glyph(UNSCALED_POINTER_ADD(font, (40 * c)));
    }
}

void print_string(char* s){
    while(*s){
        if(*s == '\n'){
            line += 1;
            cursor = 0;
            s++;
            continue;
        }
        _print_glyph(UNSCALED_POINTER_ADD(font, (40 * (*s++))));
    }
}

void test_font(){
    PDEBUG("_binary_font_psf_start: 0x%x\n", _binary_src_kernel_fonts_tamzen10x20_psf_start);
    PDEBUG("_binary_font_psf_end: 0x%x\n", _binary_src_kernel_fonts_tamzen10x20_psf_end);
    PDEBUG("_binary_font_psf_size: 0x%x\n", _binary_src_kernel_fonts_tamzen10x20_psf_size);

    psf2_header_t* font_hdr = (psf2_header_t*)_binary_src_kernel_fonts_tamzen10x20_psf_start;
    font = UNSCALED_POINTER_ADD(font, font_hdr->headersize);

    if (font_hdr->magic != 0x864ab572) {
        PDEBUG("Not a PSF2 font!");
    }else{
        PDEBUG("Loaded PSF2 font.\n");
    }
}