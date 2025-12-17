#ifndef __MAILBOX_H__
#define __MAILBOX_H__

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

typedef struct {
    uint32_t buffer[256] __attribute__((aligned(16)));
} mailbox_buf_t;


uint32_t mailbox_read(uint32_t channel);
void mailbox_write(uint32_t msg, uint32_t channel);

#endif