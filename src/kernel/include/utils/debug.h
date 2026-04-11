#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "macros.h"

typedef union {
    u64 value;
    struct {
        u64 ti        : 2;  // [1:0]
        u64 rv        : 1;  // [2]
        u64 res0      : 2;  // [4:3]
        u64 rn        : 5;  // [9:5]
        u64 res1      : 10;  // [19:10]
        u64 cond      : 4;  // [23:20]
        u64 cv        : 1;  // [24]
        u64 il        : 1;  // [25]
        u64 ec        : 6;  // [31:26]
    } __attribute__((packed)) bits;
} esr_el1_t;


#endif