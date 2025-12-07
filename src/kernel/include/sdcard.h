#ifndef __SDCARD_H__
#define __SDCARD_H__

#include "peripherals/base.h"

#define SDHOST_BASE = (PBASE + 0x300000)

typedef struct sdhost_s{
    
} sdhost_t;

void init_sd();
void read_sd();

#endif