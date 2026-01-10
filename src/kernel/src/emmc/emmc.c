#include "macros.h"
#include "io/kprintf.h"
#include "io/gpio.h"
#include "emmc/emmc.h"
#include "emmc/emmc_clock.h"
#include "utils/timer.h"

static emmc_device_t device = {0};
static const emmc_cmd_t INVALID_CMD = RES_CMD;

static uint32_t sd_error_mask(sd_error_t err){
    return 1 << (16 + (uint32_t) err);
}

static void set_last_error(uint32_t intr_val){
    device.last_error = intr_val & 0xFFFF0000;
    device.last_interrupt = intr_val;
}

static bool do_data_transfer(emmc_cmd_t cmd){
    uint32_t wrIrpt = 0;
    bool write = FALSE;
    if(cmd.direction){
        wrIrpt = (1 << 5);
    }else{
        wrIrpt = (1 << 4);
        write = TRUE;
    }

    uint32_t* data = (uint32_t*) device.buffer;

    for(int block = 0; block < device.transfer_blocks; block++){
        wait_reg_mask(&EMMC->int_flags, wrIrpt | 0x8000, TRUE, 2000);
        uint32_t intr_val = EMMC->int_flags;
        EMMC->int_flags = wrIrpt | 0x8000;

        if((intr_val & 0xFFFF0000)){
            set_last_error(intr_val);
            return FALSE;
        }

        uint32_t length = device.block_size;
        if(write){
            for(; length > 0; length -= 4){
                EMMC->data = *data++;
            }
        }else{
            for (; length > 0; length -= 4) {
                *data++ = EMMC->data;
            }
        }
    }
    return TRUE;
}

static bool emmc_issue_command(emmc_cmd_t cmd, uint32_t arg, uint32_t timeout) {
    device.last_command_value = TO_REG(&cmd);
    reg32_t command_reg = device.last_command_value;

    if (device.transfer_blocks > 0xFFFF) {
        PDEBUG("EMMC_ERR: transferBlocks too large: %d\n", device.transfer_blocks);
        return FALSE;
    }

    EMMC->block_size_count = device.block_size | (device.transfer_blocks << 16);
    EMMC->arg1 = arg;
    EMMC->cmd_xfer_mode = command_reg;

    timer_sleep(10);

    int times = 0;

    while(times < timeout) {
        uint32_t reg = EMMC->int_flags;

        if (reg & 0x8001) {
            break;
        }

        timer_sleep(1);
        times++;
    }

    if (times >= timeout) {
        //just doing a warn for this because sometimes it's ok.
        PDEBUG("EMMC_WARN: emmc_issue_command timed out\n");
        device.last_success = FALSE;
        return FALSE;
    }

    uint32_t intr_val = EMMC->int_flags;

    EMMC->int_flags = 0xFFFF0001;

    if ((intr_val & 0xFFFF0001) != 1) {

        PDEBUG("EMMC_DEBUG: Error waiting for command interrupt complete: %d\n", cmd.index);

        set_last_error(intr_val);

        PDEBUG("EMMC_DEBUG: IRQFLAGS: 0x%x - 0x%x - 0x%x\n", EMMC->int_flags, EMMC->status, intr_val);

        device.last_success = FALSE;
        return FALSE;
    }

    switch(cmd.response_type) {
        case RT48:
        case RT48Busy:
            device.last_response[0] = EMMC->response[0];
            break;

        case RT136:
            device.last_response[0] = EMMC->response[0];
            device.last_response[1] = EMMC->response[1];
            device.last_response[2] = EMMC->response[2];
            device.last_response[3] = EMMC->response[3];
            break;
    }

    if (cmd.is_data) {
        do_data_transfer(cmd);
    }

    if (cmd.response_type == RT48Busy || cmd.is_data) {
        wait_reg_mask(&EMMC->int_flags, 0x8002, TRUE, 2000);
        intr_val = EMMC->int_flags;

        EMMC->int_flags = 0xFFFF0002;

        if ((intr_val & 0xFFFF0002) != 2 && (intr_val & 0xFFFF0002) != 0x100002) {
            set_last_error(intr_val);
            return FALSE;
        }

        EMMC->int_flags = 0xFFFF0002;
    }

    device.last_success = TRUE;

    return TRUE;
}

static bool emmc_app_command(uint32_t command, uint32_t arg, uint32_t timeout) {

    if (commands[command].index >= 60) {
        PDEBUG("EMMC_ERR: INVALID APP COMMAND\n");
        return FALSE;
    }

    device.last_command = commands[CTApp];

    uint32_t rca = 0;

    if (device.rca) {
        rca = device.rca << 16;
    }

    if (emmc_issue_command( device.last_command, rca, 2000)) {
        device.last_command = commands[command];

        return emmc_issue_command( device.last_command, arg, 2000);
    }

    return FALSE;
}

static bool emmc_command(uint32_t command, uint32_t arg, uint32_t timeout){
    if (command & 0x80000000) {
        //The app command flag is set, shoudl use emmc_app_command instead.
        PDEBUG("EMMC_ERR: COMMAND ERROR NOT APP\n");
        return FALSE;
    }

    device.last_command = commands[command];

    if (TO_REG(&device.last_command) == TO_REG(&INVALID_CMD)) {
        PDEBUG("EMMC_ERR: INVALID COMMAND!\n");
        return FALSE;
    }

    return emmc_issue_command( device.last_command, arg, timeout);
}

static bool reset_command() {
    EMMC->control[1] |= EMMC_CTRL1_RESET_CMD;

    for (int i = 0; i < 10000; i++) {
        if (!( EMMC->control[1] & EMMC_CTRL1_RESET_CMD)) {
            return TRUE;
        }
        timer_sleep(1);
    }

    PDEBUG("EMMC_ERR: Command line failed to reset properly: 0x%x\n",  EMMC->control[1]);
    return FALSE;
}

static bool check_ocr(){
    bool passed = FALSE;
    for(int i = 0; i < 5; i++){
        if(!emmc_app_command(CTOcrCheck, 0, 2000)){
            PDEBUG("EMMC_WARNING: APP CMD OCR CHECK TRY %d FAILED.\n", i + 1);
            passed = FALSE;
        }else{
            passed = TRUE;
        }

        if(passed) break;
    }

    if(!passed){
        PDEBUG("EMMC_ERR: CMD 41 FAILED\n");
        return FALSE;
    }

    device.ocr = ((device.last_response[0] >> 8) & 0xFFFF);

    PDEBUG("MEMORY OCR: %x\n", device.ocr); 

    return TRUE;
}

static bool check_usable_card(){
    if(!emmc_command(CTIOSetOpCond, 0, 1000)){
        if(device.last_error == 0){
            PDEBUG("EMMC_ERROR: CTIOSetOpCond timeout\n");
        }else if(device.last_error & (1 << 16)){
            if(!reset_command()){
                return FALSE;
            }
            EMMC->int_flags = sd_error_mask(SDECommandTimeout);
        }else{
            PDEBUG("EMMC_ERROR: SDIO card not supported.\n");
            return FALSE;
        }
    }

    return TRUE;
}

static bool check_v2_card(){
    if(!emmc_command(CTSendIfCond, 0x1AA, 200)){
        if(device.last_error & (1 << 16)){
            if(!reset_command()) return FALSE;
            EMMC->int_flags = sd_error_mask(SDECommandTimeout);
            PDEBUG("EMMC_ERROR: SEND_IF_COND command timeout.\n");
            return FALSE;
        }else{
            PDEBUG("EMMC_ERROR: Failure sending SEND_IF_COND command.\n");
            return FALSE;
        }
    }else{
        if((device.last_response[0] & 0xFFF) != 0x1AA){
            PDEBUG("EMMC_ERROR: Unusable SD Card: 0x%x\n", device.last_response);
            return FALSE;
        }
    }
    return TRUE;
}

static bool check_sdhc_support(bool v2_card){
    bool card_busy = TRUE;
    while(card_busy){
        uint32_t v2_flags = 0;

        if(v2_card) v2_flags |= (1 << 30); // SDHC support

        if(!emmc_app_command(CTOcrCheck, 0x00FF8000 | v2_flags, 2000)){
            PDEBUG("EMMC_ERROR: APP CMD 41 FAILED 2nd\n");
            return FALSE;
        }

        if((device.last_response[0] >> 31) & 1){
            device.ocr = ((device.last_response[0] >> 8) & 0xFFFF);
            device.sdhc = ((device.last_response[0] >> 30) & 1) != 0;
            card_busy = FALSE;
        }else{
            PDEBUG("EMMC_SLEEPING: 0x%x\n", device.last_response[0]);
            timer_sleep(500);
        }
    }

    return TRUE;
}

static bool check_rca() {
    if (!emmc_command( CTSendCide, 0, 2000)) {
        PDEBUG("EMMC_ERR: Failed to send CID\n");

        return FALSE;
    }

    PDEBUG("EMMC_DEBUG: CARD ID: 0x%x.0x%x.0x%x.0x%x\n", device.last_response[0], device.last_response[1], device.last_response[2], device.last_response[3]);

    if (!emmc_command( CTSendRelativeAddr, 0, 2000)) {
        PDEBUG("EMMC_ERR: Failed to send Relative Addr\n");

        return FALSE;
    }

    device.rca = (device.last_response[0] >> 16) & 0xFFFF;
    
    PDEBUG("EMMC_DEBUG: RCA: 0x%x\n", device.rca);

    PDEBUG("EMMC_DEBUG: CRC_ERR: %d\n", (device.last_response[0] >> 15) & 1);
    PDEBUG("EMMC_DEBUG: CMD_ERR: %d\n", (device.last_response[0] >> 14) & 1);
    PDEBUG("EMMC_DEBUG: GEN_ERR: %d\n", (device.last_response[0] >> 13) & 1);
    PDEBUG("EMMC_DEBUG: STS_ERR: %d\n", (device.last_response[0] >> 9) & 1);
    PDEBUG("EMMC_DEBUG: READY  : %d\n", (device.last_response[0] >> 8) & 1);

    if (!((device.last_response[0] >> 8) & 1)) {
        PDEBUG("EMMC_ERR: Failed to read RCA\n");
        return FALSE;
    }

    return TRUE;
}

static bool set_scr() {
    if (!device.sdhc) {
        if (!emmc_command( CTSetBlockLen, 512, 2000)) {
            PDEBUG("EMMC_ERR: Failed to set block len\n");
            return FALSE;
        }
    }

    uint32_t bsc = EMMC->block_size_count;
    bsc &= ~0xFFF; //mask off bottom bits
    bsc |= 0x200; //set bottom bits to 512
    EMMC->block_size_count = bsc;

    device.buffer = &device.scr.scr[0];
    device.block_size = 8;
    device.transfer_blocks = 1;

    if (!emmc_app_command( CTSendSCR, 0, 30000)) {
        PDEBUG("EMMC_ERR: Failed to send SCR\n");
        return FALSE;
    }

    PDEBUG("EMMC_DEBUG: GOT SRC: SCR0: 0x%x SCR1: 0x%x BWID: 0x%x\n", device.scr.scr[0], device.scr.scr[1], device.scr.bus_widths);

    device.block_size = 512;

    uint32_t scr0 = BSWAP32(device.scr.scr[0]);
    device.scr.version = 0xFFFFFFFF;
    uint32_t spec = (scr0 >> (56 - 32)) & 0xf;
    uint32_t spec3 = (scr0 >> (47 - 32)) & 0x1;
    uint32_t spec4 = (scr0 >> (42 - 32)) & 0x1;

    if (spec == 0) {
        device.scr.version = 1;
    } else if (spec == 1) {
        device.scr.version = 11;
    } else if (spec == 2) {

        if (spec3 == 0) {
            device.scr.version = 2;
        } else if (spec3 == 1) {
            if (spec4 == 0) {
                device.scr.version = 3;
            }
            if (spec4 == 1) {
                device.scr.version = 4;
            }
        }
    }

    PDEBUG("EMMC_DEBUG: SCR Version: %d\n", device.scr.version);

    return TRUE;
}

static bool select_card() {
    if (!emmc_command( CTSelectCard, device.rca << 16, 2000)) {
        PDEBUG("EMMC_ERR: Failed to select card\n");
        return FALSE;
    }

    PDEBUG("EMMC_DEBUG: Selected Card\n");

    uint32_t status = (device.last_response[0] >> 9) & 0xF;

    if (status != 3 && status != 4) {
        PDEBUG("EMMC_ERR: Invalid Status: %d\n", status);
        return FALSE;
    }

    PDEBUG("EMMC_DEBUG: Status: %d\n", status);

    return TRUE;
}

static bool emmc_card_reset(){
    EMMC->control[1] = EMMC_CTRL1_RESET_HOST;
    
    PDEBUG("Resetting SD card...\n");

    if(!wait_reg_mask(&EMMC->control[1], EMMC_CTRL1_RESET_ALL, FALSE, 2000)){
        PDEBUG("EMMC card reset timeout\n");
        return FALSE;
    }

    // needed by pi4 for some reason
    uint32_t c0 = EMMC->control[0];
    c0 |= (0x0F << 8);
    EMMC->control[0] = c0;
    timer_sleep(3);

    if(!emmc_setup_clock()) return FALSE;

    EMMC->int_enable = 0;
    EMMC->int_flags = 0xFFFFFFFF;
    EMMC->int_mask = 0xFFFFFFFF;

    timer_sleep(203);
    device.transfer_blocks = 0;
    device.last_command_value = 0;
    device.last_success = FALSE;
    device.block_size = 0;

    if(!emmc_command(CTGoIdle, 0, 2000)){
        PDEBUG("EMMC_ERROR: No GO_IDLE response.\n");
        return FALSE;
    }

    bool v2_card = check_v2_card();

    if(!check_usable_card()) return FALSE;
    if(!check_ocr()) return FALSE;
    if(!check_sdhc_support(v2_card)) return FALSE;

    switch_clock_rate(device.base_clock, SD_CLOCK_NORMAL);

    timer_sleep(10);

    if(!check_rca()) return FALSE;
    if(!select_card()) return FALSE;
    if(!set_scr()) return FALSE;

    EMMC->int_flags = 0xFFFFFFFF;

    PDEBUG("EMMC Card reset!\n");

    return TRUE;
}

bool emmc_init(){
    gpio_pin_set_func(34, GFInput);
    gpio_pin_set_func(35, GFInput);
    gpio_pin_set_func(36, GFInput);
    gpio_pin_set_func(37, GFInput);
    gpio_pin_set_func(38, GFInput);
    gpio_pin_set_func(39, GFInput);

    gpio_pin_set_func(48, GFAlt3);
    gpio_pin_set_func(49, GFAlt3);
    gpio_pin_set_func(50, GFAlt3);
    gpio_pin_set_func(51, GFAlt3);
    gpio_pin_set_func(52, GFAlt3);

    device.transfer_blocks = 0;
    device.last_command_value = 0;
    device.last_success = FALSE;
    device.block_size = 0;
    device.sdhc = FALSE;
    device.ocr = 0;
    device.rca = 0;
    device.offset = 0;
    device.base_clock = 0;

    bool success = FALSE;
    for(int i = 0; i < 10; i++){
        success = emmc_card_reset();
        if(success) break;

        timer_sleep(100);
        PDEBUG("WARNING: Failed to reset card, trying again...\n");
    }
    return success;
}

bool do_data_command(bool write, uint8_t *b, uint32_t bsize, uint32_t block_no) {
    if (!device.sdhc) {
        block_no *= 512;
    }

    if (bsize < device.block_size) {
        kprintf("EMMC_ERR: INVALID BLOCK SIZE: \n", bsize, device.block_size);
        return FALSE;
    }

    device.transfer_blocks = bsize / device.block_size;

    if (bsize % device.block_size) {
        kprintf("EMMC_ERR: BAD BLOCK SIZE\n");
        return FALSE;
    }

    device.buffer = b;

    cmd_type command = CTReadBlock;

    if (write && device.transfer_blocks > 1) {
        command = CTWriteMultiple;
    } else if (write) {
        command = CTWriteBlock;
    } else if (!write && device.transfer_blocks > 1) {
        command = CTReadMultiple;
    } 

    int retry_count = 0;
    int max_retries = 3;

    if (EMMC_DEBUG) kprintf("EMMC_DEBUG: Sending command: %d\n", command);

    while(retry_count < max_retries) {
        if (emmc_command( command, block_no, 5000)) {
            break;
        }

        if (++retry_count < max_retries) {
            kprintf("EMMC_WARN: Retrying data command\n");
        } else {
            kprintf("EMMC_ERR: Giving up data command\n");
            return FALSE;
        }
    }

    return TRUE;
}

int do_read(uint8_t *b, uint32_t bsize, uint32_t block_no) {
    //TODO ENSURE DATA MODE...

    if (!do_data_command( FALSE, b, bsize, block_no)) {
        kprintf("EMMC_ERR: do_data_command failed\n");
        return -1;
    }

    return bsize;
}


int emmc_read(uint8_t *buffer, uint32_t size) {
    if (device.offset % 512 != 0) {
        kprintf("EMMC_ERR: INVALID OFFSET: %d\n", device.offset);
        return -1;
    }

    uint32_t block = device.offset / 512;

    int r = do_read( buffer, size, block);

    if (r != size) {
        kprintf("EMMC_ERR: READ FAILED: %d\n", r);
        return -1;
    }

    return size;
}

void emmc_seek(uint64_t _offset) {
    device.offset = _offset;
}

void emmc_seek_sector(uint64_t _sector) {
    device.offset = _sector * 512;
}