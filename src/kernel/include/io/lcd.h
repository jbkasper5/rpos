#ifndef __LCD_H__
#define __LCD_H__

#include "macros.h"
#include "peripherals/base.h"

#define MAILBOX_BASE   (PBASE + 0xB880)
#define MAILBOX_READ   ((volatile uint32_t *)(MAILBOX_BASE + 0x00))
#define MAILBOX_STATUS ((volatile uint32_t *)(MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE  ((volatile uint32_t *)(MAILBOX_BASE + 0x20))

#define MAILBOX_FULL   0x80000000
#define MAILBOX_EMPTY  0x40000000

#define CHANNEL_PROP 8

#define TAG_SET_PHYSICAL_SIZE 0x48003
#define TAG_SET_VIRTUAL_SIZE  0x48004
#define TAG_SET_DEPTH         0x48005
#define TAG_ALLOCATE_BUFFER   0x40001
#define TAG_GET_PITCH         0x40008
#define TAG_END               0x00000000

#define MBX ((mailbox_regs_t*)(PBASE + 0xB880))

void mailbox_write(uint8_t channel, uint64_t data);
uint32_t mailbox_read(uint8_t channel);

#include "macros.h"

#define GPU_BUS_TO_ARM(addr) ((addr) & 0x3FFFFFFF)

typedef struct frame_s{
    uint32_t* fb;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
} frame_t;


extern frame_t frame;

void scroll();
int panel_ready();
int init_framebuffer();
void write_pixel(uint64_t idx, uint32_t argb);

#endif