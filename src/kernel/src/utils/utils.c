#include "utils/utils.h"

int log2_pow2(u64 num){
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

int strlen(const char* str) {
    const char* s = str;
    while (*s) s++;
    return (int)(s - str);
}

char* strtok(char* str, char delim, char** saveptr){
    // determine if it's the first call or a cached state
    char* s = str ? str : *saveptr;
    if(!s) return NULL;

    // skip past any leading delimiters
    while(*s == delim) s++;

    // reached the end of the string
    if(*s == '\0') { *saveptr = s; return NULL; } 
    char* token = s;
    char c;

    // match tokens and separate strings
    while((c = *s)){
        if(c == delim){
            *s = '\0';
            *saveptr = s + 1;
            return token;
        }
        s++;
    }
    // if we made it here and c is NULL, then its the natural end of a non-empty string
    if(!c) { *saveptr = s; return token; } 
    
    return NULL;
}