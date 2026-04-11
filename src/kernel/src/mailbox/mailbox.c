#include "mailbox/mailbox.h"

typedef struct {
    u32 size;
    u32 code;
    u8 tags[0];
} property_buffer;

static u32 property_data[8192] __attribute__((aligned(16)));

void mailbox_write(u8 channel, u64 data) {
    while(MBX->status & MAILBOX_FULL);
    MBX->write = ((data & 0xFFFFFFF0) | (channel & 0xF));
}

u32 mailbox_read(u8 channel) {
    while(TRUE) {
        while(MBX->status & MAILBOX_EMPTY);

        u32 data = MBX->read;

        u8 read_channel = (u8)(data & 0xF);

        if (read_channel == channel) {
            return (data & 0xFFFFFFF0);
        }
    }
}

bool mailbox_process(mailbox_tag *tag, u32 tag_size) {
    int buffer_size = tag_size + 12;

    memcpy(&property_data[2], tag, tag_size);

    property_buffer *buff = (property_buffer *)property_data;
    buff->size = buffer_size;
    buff->code = RPI_FIRMWARE_STATUS_REQUEST;
    property_data[(tag_size + 12) / 4 - 1] = RPI_FIRMWARE_PROPERTY_END;

    mailbox_write(MAIL_TAGS, (u64)(void *)property_data);

    mailbox_read(MAIL_TAGS);

    memcpy(tag, property_data + 2, tag_size);

    return TRUE;
}

bool mailbox_generic_command(u32 tag_id, u32 id, u32 *value) {
    mailbox_generic mbx;
    mbx.tag.id = tag_id;
    mbx.tag.value_length = 0;
    mbx.tag.buffer_size = sizeof(mailbox_generic) - sizeof(mailbox_tag);
    mbx.id = id;
    mbx.value = *value;

    if (!mailbox_process((mailbox_tag *)&mbx, sizeof(mbx))) {
        DEBUG("FAILED TO PROCESS: %X\n", tag_id);
        return FALSE;
    }

    *value = mbx.value;

    return TRUE;
}

u32 mailbox_clock_rate(clock_type ct) {
    mailbox_clock c;
    c.tag.id = RPI_FIRMWARE_GET_CLOCK_RATE;
    c.tag.value_length = 0;
    c.tag.buffer_size = sizeof(c) - sizeof(c.tag);
    c.id = ct;

    mailbox_process((mailbox_tag *)&c, sizeof(c));

    return c.rate;
}

bool mailbox_power_check(u32 type) {
    mailbox_power p;
    p.tag.id = RPI_FIRMWARE_GET_DOMAIN_STATE;
    p.tag.value_length = 0;
    p.tag.buffer_size = sizeof(p) - sizeof(p.tag);
    p.id = type;
    p.state = ~0;

    mailbox_process((mailbox_tag *)&p, sizeof(p));

    return p.state && p.state != ~0;
}

static bool pcie_clock_on(){
    mailbox_clock mbx;
    mbx.tag.id = RPI_FIRMWARE_SET_CLOCK_STATE;
    mbx.tag.buffer_size = 8;
    mbx.tag.value_length = 8;
    
    mbx.id = 0x9;    // PCIe Clock ID
    mbx.rate = 0x1;  // State: On
    
    return mailbox_process((mailbox_tag *)&mbx, sizeof(mbx));
}

bool mailbox_pcie_usb_power_on(){
    u64* testptr = 0xFFFF8000FD50406C;
    u64* testptr2 = 0xFFFF8000FD504068;

    u64 test = *testptr;
    u64 test2 = *testptr2;

    int ret = pcie_clock_on();

    mailbox_power mbx;

    mbx.tag.id = RPI_FIRMWARE_SET_POWER_STATE;
    mbx.tag.buffer_size = 8;   // We are sending 8 bytes of data (id + state)
    mbx.tag.value_length = 8;  // We expect 8 bytes back (request bit 31 is 0)

    mbx.id = 3; // RPI_POWER_DOMAIN_VIDEO_SCALER
    mbx.state = 0x3; 
    mailbox_process((mailbox_tag *)&mbx, sizeof(mbx));

    // Then try ID 6 again
    mbx.id = 6; // RPI_POWER_DOMAIN_USB
    mbx.state = 0x3;
    mailbox_process((mailbox_tag *)&mbx, sizeof(mbx));

    if (!mailbox_process((mailbox_tag *)&mbx, sizeof(mbx))) {
        WARNING("USB Power: Mailbox call failed\n");
        return FALSE;
    }

    // 4. Verify: The VC updates the state field with the actual status
    // Bit 1 of the returned state indicates if it's powered (1 = On)
    if (!(mbx.state & 0x1)) {
        WARNING("USB Power: Hardware refused to wake up (State: %x)\n", mbx.state);
        return FALSE;
    }

    return TRUE;
}