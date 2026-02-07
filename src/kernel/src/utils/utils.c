#include "utils/utils.h"

int log2_pow2(uint64_t num){
    int ret = 0;
    if(!num) return ret;
    while(num >> ret) ret++;
    return ret - 1;
}

int strcmp(const char* str1, const char* str2){
    while(*str1 && *str2){
        if(*str1 != *str2) return TRUE;
        str1++;
        str2++;
    }
    return !(*str1 == '\0' && *str2 == '\0');
}