#include <stdarg.h>
#include "macros.h"
#include "uapi/rpos/syscall_macros.h"

typedef enum{
    ANSI_ESCAPE,
    ANSI_NORMAL,
    ANSI_PARSING,
    ANSI_END_PARSING
} ANSI_STATE;

typedef struct{
    ANSI_STATE state;
    char params[16];
    int param_len;
    char cmd;
} ansi_state;


typedef struct {
    char buf[2048];
    int size;
    int pos;
} printbuf;

printbuf buf = {
    .size = 2048,
    .pos = 0
};

static int atoi_until_semicolon(char** pp){
    char* p = *pp;
    int num = 0;

    while(*p && *p != ';'){
        num = (num * 10) + (*p - '0');
        p++;
    }

    if(*p == ';') p++; // advance past semicolon

    *pp = p; // update caller pointer
    return num;
}

static ansi_state state = {
    .state = ANSI_NORMAL,
    .param_len = 0
};


static void buf_putc(char c){
    // at the edge of the buffer, so flush it and set the buffer back
    if(buf.pos == buf.size - 1){
        buf.buf[buf.pos] = '\0';
        syscall(SYS_WRITE, 1, buf.buf);
        buf.pos = 0;
    }

    // otherwise, add the character to the buffer
    buf.buf[buf.pos++] = c;

    // if the character is a newline, flush the buffer
    if(c == '\n'){
        buf.buf[buf.pos++] = '\0';
        syscall(SYS_WRITE, 1, buf.buf);
        buf.pos = 0;
    }
}   

static ANSI_STATE ansi_normal(char c){
    if(c == '\e') return ANSI_ESCAPE;
    else buf_putc(c);
    return ANSI_NORMAL;
}

static ANSI_STATE ansi_escape(char c){
    if(c == '['){
        state.param_len = 0;
        return ANSI_PARSING;   
    }
    return ANSI_NORMAL;
}

static ANSI_STATE ansi_parsing(char c){
    if(c == 'm'){
        state.cmd = 'm';
        state.params[state.param_len] = '\0';
        return ANSI_END_PARSING;
    }
    switch(c){
        case '0' ... '9':
        case ';':
            if(state.param_len < sizeof(state.params) - 1) {
                state.params[state.param_len++] = c;
                return ANSI_PARSING;
            }
        default:
            // reset the param buffer and return
            state.param_len = 0;
            return ANSI_NORMAL;
    }
}

static ANSI_STATE ansi_end_escape(char c){
    // begin parsing the intent of the escape sequence

    int params[5];   // typical kernel can cap at 5-6 params
    int n = 0;

    char* p = state.params;
    while(*p && n < 5){
        params[n++] = atoi_until_semicolon(&p); // parse integer and advance pointer past ';'
    }

    return ANSI_NORMAL;
}

static void ansi_decoder(char c){
    switch(state.state){
        case ANSI_ESCAPE:
            state.state = ansi_escape(c); break;
        case ANSI_PARSING:
            state.state = ansi_parsing(c); break;
        case ANSI_END_PARSING:
            state.state = ansi_end_escape(c);
        default:
            state.state = ansi_normal(c); break;
    }
}

static char _nibble_to_char(char nibble){
    switch(nibble){
        case 0 ... 9:
            return '0' + nibble;
        case 10 ... 15:
            return 'A' + (nibble - 10);
        default:
            return 0;
    }
}

static void parse_and_emit_int(int num){
    if(num == 0){
        ansi_decoder('0');
        return;
    }

    // Handle negative numbers
    if(num < 0){
        ansi_decoder('-');
        num = -num;
    }

    char buf[20]; // enough for 64-bit numbers
    int i = 0;

    while(num){
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    // Emit digits most-significant first
    for(int j = i-1; j >= 0; j--){
        ansi_decoder(buf[j]);
    }
}

static void parse_and_emit_hex(unsigned long num){
    char buf[sizeof(unsigned long) * 2]; // max hex digits
    int i = 0;

    do {
        buf[i++] = _nibble_to_char(num & 0xF);
        num >>= 4;
    } while(num);

    for(int j = i - 1; j >= 0; j--){
        ansi_decoder(buf[j]);
    }
}

static void emit_string(char* s){
    while(*s){
        ansi_decoder(*s);
        s++;
    }
}


static void parse_and_emit_double(double val){
    if(val < 0){
        ansi_decoder('-');
        val = -val;
    }
    // integer part
    parse_and_emit_int((int)val);
    ansi_decoder('.');
    // fractional part to 6 decimal places
    double frac = val - (int)val;
    for(int i = 0; i < 6; i++){
        frac *= 10;
        ansi_decoder('0' + (int)frac);
        frac -= (int)frac;
    }

}

static int expand(char* s, va_list* args){
    u32 consumed_chars = 1;
    switch(*s){
        case 'd':
            parse_and_emit_int(va_arg(*args, int)); break;
        case 's':
            emit_string(va_arg(*args, char*)); break;
        case 'c':
            ansi_decoder(va_arg(*args, int)); break;
        case 'l':
            parse_and_emit_int(va_arg(*args, int)); break;
        case 'f':
            parse_and_emit_double(va_arg(*args, double)); break;
        case 'x':
            parse_and_emit_hex(va_arg(*args, unsigned long)); break;
        case '%':
            ansi_decoder(*s); break;
    }
    return consumed_chars;
}

void printf(char* format_str, ...){
    char* ptr = format_str;

    va_list list;
    va_start(list, format_str);

    while(*ptr){
        if(*ptr == '%'){
            ptr++;
            ptr += expand(ptr, &list);
        }else ansi_decoder(*ptr++);
    }

    va_end(list);
    return;
}