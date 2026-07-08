#ifndef __EMMC_TYPES_H__
#define __EMMC_TYPES_H__

#include "macros.h"

typedef struct {
    u8 resp_a : 1;
    u8 block_count : 1;
    u8 auto_command : 2;
    u8 direction : 1;
    u8 multiblock : 1;
    u16 resp_b : 10;
    u8 response_type : 2;
    u8 res0 : 1;
    u8 crc_enable : 1;
    u8 idx_enable : 1;
    u8 is_data : 1;
    u8 type : 2;
    u8 index : 6;
    u8 res1 : 2;
} emmc_cmd_t;
typedef enum {
    RTNone,
    RT136,
    RT48,
    RT48Busy
} cmd_resp_type;

typedef enum {
    CTGoIdle = 0,
    CTSendCide = 2,
    CTSendRelativeAddr = 3,
    CTIOSetOpCond = 5,
    CTSelectCard = 7,
    CTSendIfCond = 8,
    CTSetBlockLen = 16,
    CTReadBlock = 17,
    CTReadMultiple = 18,
    CTWriteBlock = 24,
    CTWriteMultiple = 25,
    CTOcrCheck = 41,
    CTSendSCR = 51,
    CTApp = 55
} cmd_type;

typedef struct {
    u32 scr[2];
    u32 bus_widths;
    u32 version;
} scr_register_t;

typedef enum {
    SDECommandTimeout,
    SDECommandCrc,
    SDECommandEndBit,
    SDECommandIndex,
    SDEDataTimeout,
    SDEDataCrc,
    SDEDataEndBit,
    SDECurrentLimit,
    SDEAutoCmd12,
    SDEADma,
    SDETuning,
    SDERsvd
} sd_error_t;

typedef struct {
    bool last_success;
    u32 transfer_blocks;
    emmc_cmd_t last_command;
    u32 last_command_value;
    u32 block_size;
    u32 last_response[4];
    bool sdhc;
    u16 ocr;
    u32 rca;
    u64 offset;
    void *buffer;
    u32 base_clock;
    u32 last_error;
    u32 last_interrupt;
    scr_register_t scr;
} emmc_device_t;

typedef struct  {
    u32 arg2;
    u32 block_size_count;
    u32 arg1;
    u32 cmd_xfer_mode;
    u32 response[4];
    u32 data;
    u32 status;
    u32 control[2];
    u32 int_flags;
    u32 int_mask;
    u32 int_enable;
    u32 control2;
    u32 cap1;
    u32 cap2;
    u32 res0[2];
    u32 force_int;
    u32 res1[7];
    u32 boot_timeout;
    u32 debug_config;
    u32 res2[2];
    u32 ext_fifo_config;
    u32 ext_fifo_enable;
    u32 tune_step;
    u32 tune_SDR;
    u32 tune_DDR;
    u32 res3[23];
    u32 spi_int_support;
    u32 res4[2];
    u32 slot_int_status;
} emmc_regs_t;

#endif
