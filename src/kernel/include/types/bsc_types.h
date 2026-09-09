#ifndef __BSC_TYPES_H__
#define __BSC_TYPES_H__

#include "macros.h"

// https://pip-assets.raspberrypi.com/categories/545-raspberry-pi-4-model-b/documents/RP-008248-DS-1-bcm2711-peripherals.pdf
// page 24

struct BSCRegs{
    union {
        reg32_t value;
        struct {
            u32 read        : 1;  // [0]
            u32 res0        : 3;  // [3:1]
            u32 clear       : 2;  // [5:4]
            u32 res1        : 1;  // [6]
            u32 st          : 1;  // [7]
            u32 intd        : 1;  // [8]
            u32 intt        : 1;  // [9]
            u32 intr        : 1;  // [10]
            u32 res2        : 4;  // [14:11]
            u32 i2cen       : 1;  // [15]
            u32 res3        : 16; // [31:16]
        } bits; 
    } c;
    union {
        reg32_t value;
        struct {
            u32 ta          : 1;  // [0]
            u32 done        : 1;  // [1]
            u32 txw         : 1;  // [2]
            u32 rxr         : 1;  // [3]
            u32 txd         : 1;  // [4]
            u32 rxd         : 1;  // [5]
            u32 txe         : 1;  // [6]
            u32 rxf         : 1;  // [7]
            u32 err         : 1;  // [8]
            u32 clkt        : 1;  // [9]
            u32 res0        : 22; // [31:10]
        } bits; 
    } s;
    union {
        reg32_t value;
        struct {
            u32 dlen        : 16; // [15:0]
            u32 res0        : 16; // [31:16]
        } bits; 
    } dlen;
    union {
        reg32_t value;
        struct {
            u32 addr        : 7;  // [6:0]
            u32 res0        : 25; // [31:7]
        } bits; 
    } a;
    union {
        reg32_t value;
        struct {
            u32 data        : 8;  // [7:0]
            u32 res0        : 24; // [31:8]
        } bits; 
    } fifo;
    union {
        reg32_t value;
        struct {
            u32 cdiv        : 16; // [15:0]
            u32 res0        : 16; // [31:16]
        } bits; 
    } div;
    union {
        reg32_t value;
        struct {
            u32 redl        : 16; // [15:0]
            u32 fedl        : 16; // [31:16]
        } bits; 
    } del;
    union {
        reg32_t value;
        struct {
            u32 tout        : 16; // [15:0]
            u32 res0        : 16; // [31:16]
        } bits; 
    } clkt;
};

#endif
