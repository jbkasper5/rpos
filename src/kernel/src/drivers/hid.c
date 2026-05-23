/*
 * hid.c - HID boot-protocol layer.
 *
 * Two things happen here:
 *
 *   1. hid_attach: allocate a small DMA buffer for the device's report
 *      and queue the first interrupt-IN transfer. The xHCI controller
 *      will repeatedly clock out this transfer at the endpoint's polling
 *      interval until we cancel it.
 *
 *   2. hid_dispatch_report (called by xhci.c after each completion): pass
 *      the raw bytes to either a keyboard or mouse handler depending on
 *      the device's interface protocol, then immediately re-queue another
 *      IN transfer.
 *
 * The keyboard handler tracks the set of currently-pressed Usage IDs and
 * emits press/release callbacks - both are TODOs for you. The mouse
 * handler likewise emits dx/dy/buttons callbacks. Wire whichever event
 * sink (your CLI, a syscall, a ringbuffer) you want to use into the
 * marked TODO spots.
 */

#include "macros.h"
#include "io/kprintf.h"
#include "memory/kmalloc.h"
#include "drivers/usb.h"
#include "drivers/xhci.h"
#include "drivers/hid.h"

/* Per-device polling state. Indexed by usb_device.slot_id. */
typedef struct {
    bool         in_use;
    usb_device_t* dev;
    u8*          buf;        /* DMA buffer the xHCI is told to write into */
    u32          buf_len;
    hid_kbd_report_t prev_kbd;    /* previous report for edge-detection */
} hid_state_t;

static hid_state_t g_hid[256];


/* ----------------------------------------------------------------------- *
 * Keyboard report parser.
 *
 * Boot report (8 bytes):
 *   [0]    modifier mask
 *   [1]    reserved
 *   [2..7] up to 6 simultaneously-pressed Usage IDs (0 = empty slot)
 *
 * "Press" and "release" are derived by diffing against the previous report.
 *
 * TODO(you): wire the two callouts at the bottom into whatever event sink
 * makes sense for the rest of the OS (your CLI's keyboard queue, a /dev/input
 * style ring buffer, etc.). The translation from USB Usage ID to ASCII/scan
 * code lives in a lookup table you'll want to add - the standard reference
 * is the table in chapter 10 of "HID Usage Tables 1.4" (Usage Page 0x07).
 */
static void parse_keyboard(hid_state_t* hs, u8* buf, u32 len) {
    if (len < 8) return;
    hid_kbd_report_t* curr = (hid_kbd_report_t*)buf;
    hid_kbd_report_t* prev = &hs->prev_kbd;

    /* Releases: keys in prev but not in curr. */
    for (int i = 0; i < 6; i++) {
        u8 k = prev->keys[i];
        if (!k) continue;
        bool still = FALSE;
        for (int j = 0; j < 6; j++) if (curr->keys[j] == k) { still = TRUE; break; }
        if (!still) {
            /* TODO(you): emit "key released" event for usage `k`. */
            DEBUG("kbd: release usage=0x%x mods=0x%x\n", k, curr->modifier);
        }
    }

    /* Presses: keys in curr but not in prev. */
    for (int i = 0; i < 6; i++) {
        u8 k = curr->keys[i];
        if (!k) continue;
        bool was = FALSE;
        for (int j = 0; j < 6; j++) if (prev->keys[j] == k) { was = TRUE; break; }
        if (!was) {
            /* TODO(you): emit "key pressed" event for usage `k`. Convert
             * to ASCII via your usage table; respect the modifier mask
             * for shift/ctrl/alt. */
            INFO("kbd: press usage=0x%x mods=0x%x\n", k, curr->modifier);
        }
    }

    *prev = *curr;
}


/* ----------------------------------------------------------------------- *
 * Mouse report parser.
 *
 * Boot report is 3 bytes (some mice have a 4th wheel byte). dx/dy are
 * signed int8 deltas - sum them into your cursor state. Buttons bitmap
 * is edge-detected against the previous report.
 *
 * TODO(you): same as the keyboard - decide where mouse events go.
 */
static void parse_mouse(hid_state_t* hs, u8* buf, u32 len) {
    (void)hs;
    if (len < 3) return;
    hid_mouse_report_t* m = (hid_mouse_report_t*)buf;
    s8 wheel = (len >= 4) ? m->wheel : 0;

    INFO("mouse: btn=0x%x dx=%d dy=%d wheel=%d\n",
         m->buttons, m->dx, m->dy, wheel);
    /* TODO(you): emit a delta event into your mouse sink. */
}


/* ----------------------------------------------------------------------- *
 * Public entry points.
 * ----------------------------------------------------------------------- */
void hid_attach(usb_device_t* dev) {
    hid_state_t* hs = &g_hid[dev->slot_id];
    if (hs->in_use) {
        WARNING("hid: slot %d already attached\n", dev->slot_id);
        return;
    }

    /* The boot keyboard report is 8 bytes; the boot mouse report is 3.
     * Allocate the endpoint's MPS just to be safe (typically 8 for both). */
    u32 buf_len = dev->ep_in_mps;
    if (buf_len < 8) buf_len = 8;

    /* xHCI requires a DMA-safe buffer. */
    u8* buf = (u8*)xhci_dma_alloc(buf_len, 64);
    if (!buf) {
        ERROR("hid: failed to allocate report buffer\n");
        return;
    }

    hs->in_use = TRUE;
    hs->dev = dev;
    hs->buf = buf;
    hs->buf_len = buf_len;

    /* Endpoint number 1 in, dir=IN. DCI = 2*ep_num + 1. */
    u8 ep_num = dev->ep_in_addr & USB_ENDPOINT_NUMBER_MASK;
    u8 dci    = (ep_num * 2) + 1;
    xhci_queue_interrupt_in(dev->slot, dci, buf, buf_len);
}


void hid_detach(usb_device_t* dev) {
    hid_state_t* hs = &g_hid[dev->slot_id];
    if (!hs->in_use) return;
    hs->in_use = FALSE;
    /* TODO(you): we leak `hs->buf` here. Once you have kfree for
     * buddy_alloc'd pages wire it in. */
    hs->buf = NULL;
}


/* Called by xhci.c on each Transfer Event for one of our periodic IN
 * endpoints. Parses the report then immediately re-queues another IN. */
void hid_dispatch_report(usb_device_t* dev, u8* buf, u32 len) {
    hid_state_t* hs = &g_hid[dev->slot_id];
    if (!hs->in_use) return;

    if (dev->interface_protocol == USB_HID_PROTO_KEYBOARD) {
        parse_keyboard(hs, buf, len);
    } else if (dev->interface_protocol == USB_HID_PROTO_MOUSE) {
        parse_mouse(hs, buf, len);
    }

    /* Re-arm the next periodic IN. */
    u8 ep_num = dev->ep_in_addr & USB_ENDPOINT_NUMBER_MASK;
    u8 dci    = (ep_num * 2) + 1;
    xhci_queue_interrupt_in(dev->slot, dci, hs->buf, hs->buf_len);
}
