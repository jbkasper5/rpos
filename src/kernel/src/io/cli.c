#include "io/cli.h"
#include "memory/mem.h"

#define EMPTY_GLYPH ' '
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_BLACK 0xFF000000
#define COLOR_GREEN 0xFF24BF57

extern unsigned char _binary_src_kernel_fonts_tamzen10x20_psf_end[];
extern unsigned char _binary_src_kernel_fonts_tamzen10x20_psf_size[];
extern unsigned char _binary_src_kernel_fonts_tamzen10x20_psf_start[];

unsigned char* font = _binary_src_kernel_fonts_tamzen10x20_psf_start;
unsigned long font_size;

u64 line = 0;
u64 cursor = 0;

u32 BACKGROUND_COLOR = COLOR_BLACK;
u32 DEFAULT_BACKGROUND_COLOR = COLOR_BLACK;

u32 TEXT_COLOR = COLOR_WHITE;
u32 DEFAULT_TEXT_COLOR = COLOR_WHITE;

void fill_screen(frame_t* frame, u32 argb){
    // frame width = 800
    // frame hieght = 480
    // sizeof(argb) = 4
    memset(frame->fb, argb, frame->width * frame->height * sizeof(argb));
}

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
    u32 color;

    int bytes_per_row = 2;  // 10 pixels → 2 bytes
    for (int row = 0; row < 20; row++){
        // Read the 2 bytes of this row
        u16 r = (glyph_location[row * bytes_per_row] << 8) | 
                      glyph_location[row * bytes_per_row + 1];

        u16 mask = 1 << 15;  // start at leftmost bit (MSB = pixel 0)
        for (int col = 0; col < 10; col++){
            if (r & mask) color = TEXT_COLOR;  // black pixel
            else          color = BACKGROUND_COLOR;  // white pixel

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
    }else if(c == '\t'){
        // skip 4 spaces for tab
        cursor += 40;
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

void load_font(){
    DEBUG("_binary_font_psf_start: 0x%x\n", _binary_src_kernel_fonts_tamzen10x20_psf_start);
    DEBUG("_binary_font_psf_end: 0x%x\n", _binary_src_kernel_fonts_tamzen10x20_psf_end);

    font_size = (u64) _binary_src_kernel_fonts_tamzen10x20_psf_end - (u64)_binary_src_kernel_fonts_tamzen10x20_psf_start;
    DEBUG("_binary_font_psf_size: 0x%x\n", font_size);

    psf2_header_t* font_hdr = (psf2_header_t*)_binary_src_kernel_fonts_tamzen10x20_psf_start;
    font = UNSCALED_POINTER_ADD(font, font_hdr->headersize);

    if (font_hdr->magic != 0x864ab572) {
        DEBUG("Not a PSF2 font!");
    }else{
        DEBUG("Loaded PSF2 font.\n");
    }
}

void set_text_background_color(u32 new_color){
    BACKGROUND_COLOR = 0xFF000000 | new_color;
}

void unset_text_background_color(){
    BACKGROUND_COLOR = DEFAULT_BACKGROUND_COLOR;
}

void set_text_color(u32 new_color){
    TEXT_COLOR = 0xFF000000 | new_color;
}

void unset_text_color(){
    TEXT_COLOR = DEFAULT_TEXT_COLOR;
}

void scroll(){
    // get number of consecutive bytes that make up one line on the screen
    // 800 pixels * size per pixel for 1 row
    // multiply by 20 since thats the height of 1 glyph
    u32 line_idxs = 800 * 20;
    u64 line_bytes = line_idxs * 4;
    
    memcpy(frame.fb, UNSCALED_POINTER_ADD(frame.fb, line_bytes), line_bytes * (NUM_LINES - 1));

    u32* last_line = UNSCALED_POINTER_ADD(frame.fb, (line_bytes * (NUM_LINES - 1)));

    memset(last_line, DEFAULT_BACKGROUND_COLOR, line_idxs * sizeof(u32));
}