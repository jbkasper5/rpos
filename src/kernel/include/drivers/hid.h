#ifndef __DRIVERS_HID_H__
#define __DRIVERS_HID_H__

/*
 * HID boot-protocol layer.
 *
 * We intentionally do NOT parse the full HID Report Descriptor here. That
 * descriptor is a stack-based mini-language and supporting it is a project
 * of its own. Instead, we ask devices (during enumeration in usb.c) to use
 * the "boot protocol" via SET_PROTOCOL, which forces keyboards and mice to
 * emit a fixed-layout report:
 *
 *   Keyboard boot report (8 bytes):
 *     [0] modifier mask:  LCtrl(1) LShift(2) LAlt(4) LGUI(8)
 *                         RCtrl(16) RShift(32) RAlt(64) RGUI(128)
 *     [1] reserved (0)
 *     [2..7] up to 6 simultaneously-pressed USB HID Usage IDs (0 = empty)
 *
 *   Mouse boot report (3 bytes, sometimes 4 with wheel):
 *     [0] buttons:  bit0 = left, bit1 = right, bit2 = middle
 *     [1] X movement (signed int8)
 *     [2] Y movement (signed int8)
 *     [3] wheel     (signed int8, optional)
 *
 * Each report arrives via a periodic interrupt-IN transfer that xhci.c
 * queues up. On completion the event is dispatched into one of the two
 * parsers below, which is where YOU implement the translation into your
 * preferred kbd/mouse event channel.
 */

#include "macros.h"
#include "drivers/usb.h"

/* Signed 8-bit (mouse deltas). macros.h only defines unsigned types. */
typedef signed char s8;

typedef struct PACKED {
    u8 modifier;
    u8 reserved;
    u8 keys[6];
} hid_kbd_report_t;

typedef struct PACKED {
    u8 buttons;
    s8 dx;
    s8 dy;
    s8 wheel;
} hid_mouse_report_t;

/* USB HID modifier bit positions (boot keyboard report byte 0) */
#define HID_MOD_LCTRL   0x01
#define HID_MOD_LSHIFT  0x02
#define HID_MOD_LALT    0x04
#define HID_MOD_LGUI    0x08
#define HID_MOD_RCTRL   0x10
#define HID_MOD_RSHIFT  0x20
#define HID_MOD_RALT    0x40
#define HID_MOD_RGUI    0x80

/* The hid layer doesn't need its own init; usb.c calls these directly when
 * a HID interface is found and when its periodic IN endpoint produces data. */
void hid_attach(usb_device_t* dev);
void hid_detach(usb_device_t* dev);
void hid_dispatch_report(usb_device_t* dev, u8* buf, u32 len);

#endif
