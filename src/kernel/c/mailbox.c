#include "mailbox.h"

void mailbox_write(uint32_t msg, uint32_t channel) {
    while(*MAILBOX_STATUS & MAILBOX_FULL);
    *MAILBOX_WRITE = (msg & ~0xF) | (channel & 0xF);
}

uint32_t mailbox_read(uint32_t channel) {
    uint32_t val;
    do {
        while(*MAILBOX_STATUS & MAILBOX_EMPTY);
        val = *MAILBOX_READ;
    } while((val & 0xF) != channel);
    return val & 0xFFFFFFF0;
}
