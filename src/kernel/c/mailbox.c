#include "mailbox.h"
#include "peripherals/base.h"
#include "print.h"
#include "mem.h"

static uint32_t property_data[8192] __attribute__((aligned(16)));

static void mailbox_write(uint8_t channel, uint32_t data){
    while(REGS_MAILBOX->status & MAIL_FULL);
    REGS_MAILBOX->write = ((data & 0xFFFFFFF0) | (channel & 0xF));
}

static int mailbox_read(uint8_t channel){
    while(TRUE){
        while(REGS_MAILBOX->status & MAIL_EMPTY);
        uint32_t data = REGS_MAILBOX->read;
        uint8_t read_channel = (uint8_t)(data & 0xF);
        if(read_channel == channel) return (data & 0xFFFFFFF0);
    }
}

bool mailbox_process(mailbox_tag_t* tag, uint32_t tag_size){
    // 12 to account for int and pointer in a struct
    int buffer_size = tag_size + 12;

    // 2 to skip over size and code
    memcpy(property_data + 2, tag, tag_size);

    property_buffer_t* buf = (property_buffer_t*)property_data;
    buf->size = buffer_size;
    buf->code = RPI_FIRMWARE_STATUS_REQUEST;
    property_data[(tag_size + 12) / 4 - 1] = RPI_FIRMWARE_PROPERTY_END;

    mailbox_write(MAIL_TAGS, (uint32_t)(void*)property_data);

    mailbox_read(MAIL_TAGS);

    memcpy(tag, property_data + 2, tag_size);

    return TRUE;
}

bool mailbox_generic_command(uint32_t tag_id, uint32_t id, uint32_t* value){
    mailbox_generic_t mbx;
    mbx.tag.id = tag_id;
    mbx.tag.value_length = 0;
    mbx.tag.buffer_size = sizeof(mailbox_generic_t) - sizeof(mailbox_tag_t);
    mbx.id = id;
    mbx.value = *value;

    if(!(mailbox_process((mailbox_tag_t*) &mbx, sizeof(mbx)))){
        printf("FAILED TO PROCESS MAILBOX REQUEST %x\n", tag_id);
        return FALSE;
    }
    *value = mbx.value;
    return TRUE;
}

uint32_t mailbox_clock_rate(clock_type_t ct){
    mailbox_clock_t c;
    c.tag.id = RPI_FIRMWARE_GET_CLOCK_RATE;
    c.tag.value_length = 0;
    c.tag.buffer_size = sizeof(c) - sizeof(c.tag);
    c.id = ct;

    mailbox_process((mailbox_tag_t*) &c, sizeof(c));

    return c.rate;
}

bool mailbox_power_check(uint32_t type){
    mailbox_power_t p;
    p.tag.id = RPI_FIRMWARE_GET_DOMAIN_STATE;
    p.tag.value_length = 0;
    p.tag.buffer_size = sizeof(p) - sizeof(p.tag);
    p.id = type;
    p.state = ~0;

    mailbox_process((mailbox_tag_t*) &p, sizeof(p));
    return p.state && p.state != ~0;
}