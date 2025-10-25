#ifndef __ENTRY_H__
#define __ENTRY_H__

#define SYNC_INVALID_EL1T           0
#define IRQ_INVALID_EL1T            1
#define FIQ_INVALUD_EL1T            2
#define ERROR_INVALID_EL1T          3

#define SYNC_INVALID_EL1H           4
#define IRQ_INVALID_EL1H            5
#define FIQ_INVALUD_EL1H            6
#define ERROR_INVALID_EL1H          7

#define SYNC_INVALID_EL0_64         8
#define IRQ_INVALID_EL0_64          9
#define FIQ_INVALUD_EL0_64          10
#define ERROR_INVALID_EL0_64        11

#define SYNC_INVALID_EL0_32         12
#define IRQ_INVALID_EL0_32          13
#define FIQ_INVALUD_EL0_32          14
#define ERROR_INVALID_EL0_32        15

// 31 registers, user SP, user PC, CPSR * 8 = (34 * 8) = 272
#define S_FRAME_SIZE			    272

#endif