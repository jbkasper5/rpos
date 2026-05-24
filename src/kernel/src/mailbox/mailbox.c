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

/*
 * Strict variant: returns TRUE only if the GPU acknowledges BOTH the request
 * (buff->code == SUCCESS) AND the specific tag (tag->value_length high bit
 * set). Use this when you genuinely care whether the firmware did anything,
 * e.g. for NOTIFY_XHCI_RESET where silent failure leaves the VL805 dead.
 *
 * Kept separate from mailbox_process() because many existing callers in the
 * kernel rely on the always-TRUE return semantics (framebuffer, clocks,
 * power-domain readback) - tightening the contract universally caused
 * downstream callers to mishandle the new failure path and reset the SoC
 * before kernel_main returned.
 */
bool mailbox_process_strict(mailbox_tag *tag, u32 tag_size) {
    if (!mailbox_process(tag, tag_size)) return FALSE;

    property_buffer *buff = (property_buffer *)property_data;
    if (buff->code != RPI_FIRMWARE_STATUS_SUCCESS) {
        DEBUG("mailbox: buffer code = 0x%x (request id=0x%x)\n",
              buff->code, tag->id);
        return FALSE;
    }
    if (!(tag->value_length & 0x80000000u)) {
        DEBUG("mailbox: tag 0x%x not processed (value_length=0x%x) - "
              "the GPU firmware probably doesn't support this tag\n",
              tag->id, tag->value_length);
        return FALSE;
    }
    tag->value_length &= 0x7FFFFFFFu;
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

bool mailbox_pcie_usb_power_on(){
    /* On Pi 4B the PCIe and USB blocks share a single power-rail "domain"
     * that the GPU firmware tracks under the SET_DOMAIN_STATE tag (0x38030).
     *
     * Two important nits:
     *   - Don't use SET_POWER_STATE (0x28001). That tag's IDs are device
     *     IDs (SD=0, UART0=1, UART1=2, USB HCD=3, I2C0=4, I2C1=5, I2C2=6,
     *     SPI=7, ...). The number "6" there is I2C2, NOT USB.
     *   - There is no PCIe clock exposed via SET_CLOCK_STATE on BCM2711;
     *     PCIe clocking is internal to the SoC. Don't try to "turn it on".
     *
     * If start4.elf has already powered the rail (the typical case on a
     * stock-bootloader Pi 4B), the firmware just reports back "still on"
     * and we're done.
     */
    mailbox_power mbx;
    mbx.tag.id           = RPI_FIRMWARE_SET_DOMAIN_STATE;
    mbx.tag.buffer_size  = 8;
    mbx.tag.value_length = 8;
    mbx.id    = 6;       /* RPI_POWER_DOMAIN_USB on BCM2711 */
    mbx.state = 0x3;     /* bit0 = on, bit1 = wait-until-stable */

    if (!mailbox_process((mailbox_tag *)&mbx, sizeof(mbx))) {
        WARNING("USB Power: Mailbox call failed\n");
        return FALSE;
    }

    /* Returned state.bit0 = 1 => powered. If bit0 is 0 the firmware
     * refused, which on Pi 4B usually means the GPU bootloader wasn't
     * told to expose PCIe (check config.txt). */
    if (!(mbx.state & 0x1)) {
        WARNING("USB Power: domain still off after request (state=0x%x)\n",
                mbx.state);
        return FALSE;
    }

    DEBUG("USB Power: domain on (state=0x%x)\n", mbx.state);
    return TRUE;
}