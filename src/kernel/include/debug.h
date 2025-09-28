#ifndef __DEBUG_H__
#define __DEBUG_H__

typedef union {
    uint64_t value;
    struct {
        uint64_t ti        : 2;  // [1:0]
        uint64_t rv        : 1;  // [2]
        uint64_t res0      : 2;  // [4:3]
        uint64_t rn        : 5;  // [9:5]
        uint64_t res1      : 10;  // [19:10]
        uint64_t cond      : 4;  // [23:20]
        uint64_t cv        : 1;  // [24]
        uint64_t il        : 1;  // [25]
        uint64_t ec        : 6;  // [31:26]
    } __attribute__((packed)) bits;
} esr_el1_t;


#endif