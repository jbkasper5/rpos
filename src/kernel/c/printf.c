#include "printf.h"

#define BUFSIZE         128
char conversion_buffer[BUFSIZE];



uint64_t get_sp(void) {
    uint64_t sp;
    __asm__("mov %0, sp" : "=r"(sp));
    return sp;
}

char _nibble_to_char(char nibble){
    switch(nibble){
        case 0 ... 9:
            return '0' + nibble;
        case 10 ... 15:
            return 'A' + (nibble - 10);
    }
    return 0;
}

char* _int_to_str(int num, char is_long){
    char IS_NEGATIVE = FALSE;
    if(num < 0){
        IS_NEGATIVE = TRUE;
        num = (~num) + 1;
    }

    char* str = conversion_buffer;
    str[BUFSIZE - 1] = '\0';
    int loc = BUFSIZE - 2;

    if(num == 0){
        str[loc--] = '0';
    }

    while(num){
        str[loc--] = '0' + num % 10;
        num /= 10;
    }

    if (IS_NEGATIVE) str[loc--] = '-';
    return (str + loc + 1);
}

char* _to_hex_str(unsigned long num){
    char* str = conversion_buffer;
    str[BUFSIZE - 1] = '\0';
    int loc = BUFSIZE - 2;

    if(num == 0){
        str[loc--] = '0';
    }

    while(num){
        str[loc--] = _nibble_to_char(num & 0xF);
        num >>= 4;
    }
    return (str + loc + 1);
}

void printf(char* format_str, ...){
    char* ptr = format_str;
    int nargs = 1;
    while(*ptr){
        if(*ptr == '%' && *(ptr + 1) != '%' && *(ptr + 1) != '\0') nargs++;
        ptr++;
    }

    // if no formats were specified, just print the string as normal
    if(nargs == 1){
        uart_puts(format_str);
        return;
    }

    va_list list;
    va_start(list, format_str);

    ptr = format_str;
    while(*ptr){
        if(*ptr == '%' && *(ptr + 1) != '%' && *(ptr + 1) != '\0'){
            char* str_conversion = NULL;
            switch (*(ptr + 1)){
                case 'd': 
                    str_conversion = _int_to_str(va_arg(list, int), FALSE); break;
                case 's':
                    str_conversion = va_arg(list, char*);
                    uart_puts(str_conversion);
                    ptr += 2;
                    continue;
                case 'c':
                    char c = va_arg(list, int) & 0xFF;
                    uart_putc(c); 
                    ptr += 2;
                    continue;
                case 'x':
                    str_conversion = _to_hex_str(va_arg(list, unsigned long)); break;
                case 'l':
                    str_conversion = _int_to_str(va_arg(list, long), TRUE); break;
                default:
                    break;
            }

            if(str_conversion){
                uart_puts(str_conversion);
            }
            ptr++;
        }else{
            uart_putc(*ptr);
        }
        ptr++;
    }
    va_end(list);
    return;
}