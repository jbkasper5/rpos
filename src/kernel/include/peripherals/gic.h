#ifndef __PERIPHERALS_GIC_H__
#define __PERIPHERALS_GIC_H__

#include "macros.h"

#define GIC_BASE        0xFF840000

struct gicd_regs {
    reg32_t gicd_ctlr;              // 0x000
    reg32_t gicd_typer;             // 0x004
    reg32_t gicd_iidr;              // 0x008
    reg32_t reserved0[29];          // 0x00C - 0x07C

    reg32_t gicd_igroupr[16];       // 0x080
    reg32_t reserved1[16];          // 0x0C0 - 0x0FC
    reg32_t gicd_isenabler[16];     // 0x100
    reg32_t reserved2[16];          // 0x140 - 0x17C

    reg32_t gicd_icenabler[16];     // 0x180
    reg32_t reserved3[16];          // 0x1C0 - 0x1FC

    reg32_t gicd_ispendr[16];       // 0x200
    reg32_t reserved4[16];          // 0x240 - 0x27C

    reg32_t gicd_icpendr[16];       // 0x280
    reg32_t reserved5[16];          // 0x2C0 - 0x2FC

    reg32_t gicd_isactiver[16];     // 0x300
    reg32_t reserved6[16];          // 0x340 - 0x37C

    reg32_t gicd_icactiver[16];     // 0x380
    reg32_t reserved7[16];          // 0x3C0 - 0x3FC

    reg32_t gicd_ipriorityr[128];   // 0x400
    reg32_t reserved8[128];         // 0x600 - 0x7FC

    reg32_t gicd_itargetsr[128];    // 0x800
    reg32_t reserved9[128];         // 0xA00 - 0xBFC

    reg32_t gicd_icfgr[32];         // 0xC00
    reg32_t reserved10[32];         // 0xC80 - 0xCFC

    reg32_t gicd_ppisr;             // 0xD00
    reg32_t gicd_spisr[15];         // 0xD04

    reg32_t reserved12[112];        // 0xD40 - 0xDFC

    reg32_t gicd_sgir;              // 0xF00
    reg32_t reserved13[3];          // 0xF04 - 0xF0C
    reg32_t gicd_cpendsgir[4];      // 0xF10
    reg32_t gicd_spendsgir[4];      // 0xF20

    reg32_t reserved14[40];          // 0xF30 - 0xFCC

    reg32_t gicd_pidr4;             // 0xFD0
    reg32_t gicd_pidr5;             // 0xFD4
    reg32_t gicd_pidr6;             // 0xFD8
    reg32_t gicd_pidr7;             // 0xFDC

    reg32_t gicd_pidr0;             // 0xFE0
    reg32_t gicd_pidr1;             // 0xFE4
    reg32_t gicd_pidr2;             // 0xFE8
    reg32_t gicd_pidr3;             // 0xFEC

    reg32_t gicd_cidr0;             // 0xFF0
    reg32_t gicd_cidr1;             // 0xFF4
    reg32_t gicd_cidr2;             // 0xFF8
    reg32_t gicd_cidr3;             // 0xFFC
};

struct gicc_regs {
    reg32_t gicc_ctlr;        // 0x000
    reg32_t gicc_pmr;         // 0x004
    reg32_t gicc_bpr;         // 0x008
    reg32_t gicc_iar;         // 0x00C
    reg32_t gicc_eoir;        // 0x010
    reg32_t gicc_rpr;         // 0x014
    reg32_t gicc_hppir;       // 0x018
    reg32_t gicc_abpr;        // 0x01C
    reg32_t gicc_aiar;        // 0x020
    reg32_t gicc_aeoir;       // 0x024
    reg32_t gicc_ahppir;      // 0x028
    reg32_t reserved1[41];    // 0x02C - 0x0CC
    reg32_t gicc_apr0;        // 0x0D0
    reg32_t reserved2[3];     // 0x0D4 - 0x0DC
    reg32_t gicc_nsapr0;      // 0x0E0
    reg32_t reserved3[6];     // 0x0E4 - 0xF8
    reg32_t gicc_iidr;        // 0x0FC
    reg32_t reserved4[960];   // 0x100 - 0xFFC
};



#define REGS_GICD ((struct gicd_regs*) (GIC_BASE + 0x1000))
#define REGS_GICC ((struct gicc_regs*) (GIC_BASE + 0x2000))
#endif