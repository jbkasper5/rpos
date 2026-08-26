#ifndef __MAILBOX_H__
#define __MAILBOX_H__

#include "macros.h"
#include "peripherals/base.h"
#include "memory/mem.h"
#include "io/kprintf.h"
#include "types/mailbox_types.h"

#define MAILBOX_BASE   (PBASE + 0xB880)
#define MAILBOX_READ   ((volatile u32 *)(MAILBOX_BASE + 0x00))
#define MAILBOX_STATUS ((volatile u32 *)(MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE  ((volatile u32 *)(MAILBOX_BASE + 0x20))

#define MAILBOX_FULL   0x80000000
#define MAILBOX_EMPTY  0x40000000

#define CHANNEL_PROP 8

#define TAG_SET_PHYSICAL_SIZE 0x48003
#define TAG_SET_VIRTUAL_SIZE  0x48004
#define TAG_SET_DEPTH         0x48005
#define TAG_ALLOCATE_BUFFER   0x40001
#define TAG_GET_PITCH         0x40008
#define TAG_END               0x00000000

#define MAIL_POWER    0x0 // Mailbox Channel 0: Power Management Interface
#define MAIL_FB       0x1 // Mailbox Channel 1: Frame Buffer
#define MAIL_VUART    0x2 // Mailbox Channel 2: Virtual UART
#define MAIL_VCHIQ    0x3 // Mailbox Channel 3: VCHIQ Interface
#define MAIL_LEDS     0x4 // Mailbox Channel 4: LEDs Interface
#define MAIL_BUTTONS  0x5 // Mailbox Channel 5: Buttons Interface
#define MAIL_TOUCH    0x6 // Mailbox Channel 6: Touchscreen Interface
#define MAIL_COUNT    0x7 // Mailbox Channel 7: Counter
#define MAIL_TAGS     0x8 // Mailbox Channel 8: Tags (ARM to VC)



#define MBX ((mailbox_regs_t*)(PBASE + 0xB880))


#define RPI_POWER_DOMAIN_I2C0		0
#define RPI_POWER_DOMAIN_I2C1		1
#define RPI_POWER_DOMAIN_I2C2		2
#define RPI_POWER_DOMAIN_VIDEO_SCALER	3
#define RPI_POWER_DOMAIN_VPU1		4
#define RPI_POWER_DOMAIN_HDMI		5
#define RPI_POWER_DOMAIN_USB		6
#define RPI_POWER_DOMAIN_VEC		7
#define RPI_POWER_DOMAIN_JPEG		8
#define RPI_POWER_DOMAIN_H264		9
#define RPI_POWER_DOMAIN_V3D		10
#define RPI_POWER_DOMAIN_ISP		11
#define RPI_POWER_DOMAIN_UNICAM0	12
#define RPI_POWER_DOMAIN_UNICAM1	13
#define RPI_POWER_DOMAIN_CCP2RX		14
#define RPI_POWER_DOMAIN_CSI2		15
#define RPI_POWER_DOMAIN_CPI		16
#define RPI_POWER_DOMAIN_DSI0		17
#define RPI_POWER_DOMAIN_DSI1		18
#define RPI_POWER_DOMAIN_TRANSPOSER	19
#define RPI_POWER_DOMAIN_CCP2TX		20
#define RPI_POWER_DOMAIN_CDP		21
#define RPI_POWER_DOMAIN_ARM		22

#define RPI_POWER_DOMAIN_COUNT		23


u32 mailbox_clock_rate(clock_type ct);
bool mailbox_generic_command(u32 tag_id, u32 id, u32 *value);
bool mailbox_power_check(u32 type);
bool mailbox_process(mailbox_tag *tag, u32 tag_size);
void mailbox_write(u8 channel, u64 data);
u32 mailbox_read(u8 channel);
bool mailbox_pcie_usb_power_on();

#endif