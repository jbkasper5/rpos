/*
 * usb.c - USB enumeration state machine + HID setup glue.
 *
 * This file is "the bus driver". It sits above xhci.c (which knows how to
 * issue control transfers and interrupt-IN transfers but knows nothing
 * about USB descriptors) and below hid.c (which knows boot reports but
 * nothing about transfers).
 *
 * Lifecycle for a freshly-attached device:
 *
 *   1. xhci.c sees a port status change (CCS = connected, CSC = changed),
 *      and calls usb_port_change(port).
 *   2. We reset the port (PORTSC.PR=1, wait for PRC=1).
 *   3. We allocate an xHCI slot via xhci_enable_slot.
 *   4. We issue an xhci_address_device (gives the device a USB address
 *      and a working EP0).
 *   5. We GET_DESCRIPTOR(DEVICE, 8) to discover EP0's actual max packet
 *      size, then GET_DESCRIPTOR(DEVICE, full) for the rest.
 *   6. GET_DESCRIPTOR(CONFIGURATION) - parses interface + endpoint
 *      descriptors, looking for an HID interface (class 0x03, boot subclass).
 *   7. SET_CONFIGURATION(<value>).
 *   8. SET_PROTOCOL(BOOT) + SET_IDLE(0): forces boot-report layout and
 *      asks the device to only emit reports on change.
 *   9. xhci_configure_endpoint for the IN endpoint.
 *  10. hid_attach -> hid.c queues the periodic IN transfer.
 *
 * Assumes a single device per root-hub port and no hubs. A hub would need
 * its own minimal class driver.
 */

#include "macros.h"
#include "io/kprintf.h"
#include "utils/timer.h"
#include "memory/kmalloc.h"
#include "drivers/usb.h"
#include "drivers/xhci.h"
#include "drivers/pcie.h"
#include "drivers/hid.h"

static pcie_dev_t   g_pcie;
static usb_device_t g_devices[8];
static u32          g_num_devices = 0;


/* Forward decl, also called by xhci.c on port-status-change events. */
void usb_port_change(u32 port);


/* ----------------------------------------------------------------------- *
 * usb_init: bring-up entry point. Called from kernel.c.
 * ----------------------------------------------------------------------- */
void usb_init(void) {
    INFO("usb: bringing up PCIe...\n");
    if (!pcie_init(&g_pcie)) {
        ERROR("usb: PCIe init failed - USB unavailable\n");
        return;
    }

    INFO("usb: bringing up xHCI...\n");
    if (!xhci_init(g_pcie.vl805_mmio_virt, g_pcie.vl805_mmio_size)) {
        ERROR("usb: xHCI init failed - USB unavailable\n");
        return;
    }

    /* Unmask the PCIe legacy IRQs at the RC. IRQs at the GIC/CPU level
     * are turned on later by irq_enable() in kernel.c. From this point
     * onward, port-status-change events start flowing through
     * xhci_handle_irq -> usb_port_change. */
    pcie_enable_legacy_irqs();

    /* Devices already attached at boot won't generate a CSC event because
     * the connect happened before the HC was running. Walk the root hub
     * once to pick those up. */
    for (u32 port = 1; port <= 4; port++) {
        usb_port_change(port);
    }
}


/* ----------------------------------------------------------------------- *
 * Port helpers.
 *
 * For now we compute PORTSC addresses inside usb.c. The right cleanup is
 * to add `volatile u32* xhci_portsc(u32 port)` to xhci.c that knows the
 * real operational-register base. TODO(you): do that.
 *
 * Until then, we recompute: xHCI capability registers live at
 * g_pcie.vl805_mmio_virt; CAPLENGTH is read from offset 0 (low byte).
 * ----------------------------------------------------------------------- */
static volatile u32* portsc_ptr(u32 port) {
    u64 cap_base = g_pcie.vl805_mmio_virt;
    u8  caplen   = (u8)(*(volatile u32*)cap_base & 0xFF);
    u64 op_base  = cap_base + caplen;
    return XHCI_PORT_REG_BASE(op_base, port);
}


/* USB 2.0 needs a 10-50 ms PORTSC.PR pulse. SuperSpeed auto-trains but
 * accepts a software reset too. Same code works for both on the VL805. */
static bool usb_reset_port(u32 port) {
    volatile u32* portsc = portsc_ptr(port);

    u32 v = *portsc;
    v |= XHCI_PORTSC_PR;
    /* Preserve sticky bits; write 1 to the *change* bits to clear them. */
    v |= XHCI_PORTSC_CSC | XHCI_PORTSC_PEC | XHCI_PORTSC_PRC | XHCI_PORTSC_WRC;
    *portsc = v;
    asm volatile("dmb sy");

    for (int i = 0; i < 500; i++) {
        if (*portsc & XHCI_PORTSC_PRC) {
            *portsc = (*portsc | XHCI_PORTSC_PRC);   /* W1C */
            DEBUG("usb: port %d reset done after %d ms\n", port, i);
            return TRUE;
        }
        timer_sleep(1);
    }
    WARNING("usb: port %d reset timeout\n", port);
    return FALSE;
}


/* PORTSC.Speed mapping per xHCI 7.2.2.1 "Default USB Speed ID Mapping". */
static usb_speed_t portsc_to_speed(u32 portsc) {
    u32 s = (portsc & XHCI_PORTSC_SPEED_MASK) >> XHCI_PORTSC_SPEED_SHIFT;
    switch (s) {
    case 1:  return USB_SPEED_FULL;
    case 2:  return USB_SPEED_LOW;
    case 3:  return USB_SPEED_HIGH;
    case 4:  return USB_SPEED_SUPER;
    default: return USB_SPEED_FULL;
    }
}


/* ----------------------------------------------------------------------- *
 * Standard-request convenience wrappers.
 * ----------------------------------------------------------------------- */
static int usb_get_descriptor(usb_device_t* dev, u8 type, u8 index,
                              u16 lang, void* buf, u16 len) {
    usb_setup_packet_t setup = {
        .bmRequestType = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
        .bRequest      = USB_REQ_GET_DESCRIPTOR,
        .wValue        = ((u16)type << 8) | index,
        .wIndex        = lang,
        .wLength       = len,
    };
    return xhci_control_transfer(dev->slot, &setup, buf);
}

static int usb_set_configuration(usb_device_t* dev, u8 cfg) {
    usb_setup_packet_t setup = {
        .bmRequestType = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
        .bRequest      = USB_REQ_SET_CONFIGURATION,
        .wValue        = cfg,
        .wIndex        = 0,
        .wLength       = 0,
    };
    return xhci_control_transfer(dev->slot, &setup, NULL);
}

static int hid_set_protocol(usb_device_t* dev, u8 iface, u8 protocol) {
    usb_setup_packet_t setup = {
        .bmRequestType = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
        .bRequest      = HID_REQ_SET_PROTOCOL,
        .wValue        = protocol,
        .wIndex        = iface,
        .wLength       = 0,
    };
    return xhci_control_transfer(dev->slot, &setup, NULL);
}

static int hid_set_idle(usb_device_t* dev, u8 iface, u8 duration_4ms) {
    /* duration of 0 = "only report on change". Best for keyboards. */
    usb_setup_packet_t setup = {
        .bmRequestType = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
        .bRequest      = HID_REQ_SET_IDLE,
        .wValue        = (u16)duration_4ms << 8,
        .wIndex        = iface,
        .wLength       = 0,
    };
    return xhci_control_transfer(dev->slot, &setup, NULL);
}


/* ----------------------------------------------------------------------- *
 * Walk a configuration descriptor and pick out the first boot-HID iface
 * with an interrupt-IN endpoint.
 * ----------------------------------------------------------------------- */
static bool parse_config_for_hid(u8* buf, u32 len,
                                 u8* out_cfg_value,
                                 u8* out_iface_num,
                                 u8* out_proto,
                                 u8* out_ep_addr,
                                 u16* out_ep_mps,
                                 u8* out_ep_interval) {
    if (len < sizeof(usb_config_descriptor_t)) return FALSE;
    usb_config_descriptor_t* cfg = (usb_config_descriptor_t*)buf;
    *out_cfg_value = cfg->bConfigurationValue;

    u32  i = cfg->bLength;
    bool in_hid_iface = FALSE;
    while (i + 2 <= len) {
        u8 dlen = buf[i + 0];
        u8 dtyp = buf[i + 1];
        if (dlen == 0) break;

        if (dtyp == USB_DESC_INTERFACE && i + dlen <= len) {
            usb_interface_descriptor_t* ifd =
                (usb_interface_descriptor_t*)(buf + i);
            in_hid_iface = (ifd->bInterfaceClass == USB_CLASS_HID
                         && ifd->bInterfaceSubClass == USB_SUBCLASS_BOOT
                         && (ifd->bInterfaceProtocol == USB_HID_PROTO_KEYBOARD
                          || ifd->bInterfaceProtocol == USB_HID_PROTO_MOUSE));
            if (in_hid_iface) {
                *out_iface_num = ifd->bInterfaceNumber;
                *out_proto     = ifd->bInterfaceProtocol;
            }
        } else if (dtyp == USB_DESC_ENDPOINT && in_hid_iface && i + dlen <= len) {
            usb_endpoint_descriptor_t* epd =
                (usb_endpoint_descriptor_t*)(buf + i);
            bool is_in  = (epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) != 0;
            bool is_int = (epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
                          == USB_ENDPOINT_XFER_INT;
            if (is_in && is_int) {
                *out_ep_addr     = epd->bEndpointAddress;
                *out_ep_mps      = epd->wMaxPacketSize & 0x07FF;
                *out_ep_interval = epd->bInterval;
                return TRUE;
            }
        }
        i += dlen;
    }
    return FALSE;
}


/* ----------------------------------------------------------------------- *
 * The enumeration ritual.
 * ----------------------------------------------------------------------- */
static void enumerate_port(u32 port) {
    if (g_num_devices >= sizeof(g_devices)/sizeof(g_devices[0])) {
        WARNING("usb: too many devices\n");
        return;
    }

    if (!usb_reset_port(port)) return;

    usb_device_t* dev = &g_devices[g_num_devices++];
    dev->port  = port;
    dev->speed = portsc_to_speed(*portsc_ptr(port));

    xhci_slot_t* slot = xhci_enable_slot();
    if (!slot) { ERROR("usb: enable_slot failed\n"); g_num_devices--; return; }
    dev->slot     = slot;
    dev->slot_id  = slot->slot_id;
    slot->usb_dev = dev;

    if (xhci_address_device(slot, port, dev->speed) < 0) {
        ERROR("usb: address_device failed\n");
        return;
    }

    /* First 8 bytes of device descriptor (learn the real EP0 MPS). */
    u8 dbuf[256] = {0};
    if (usb_get_descriptor(dev, USB_DESC_DEVICE, 0, 0, dbuf, 8) < 0) {
        ERROR("usb: GET_DESCRIPTOR(DEVICE, 8) failed\n");
        return;
    }
    usb_device_descriptor_t* ddesc = (usb_device_descriptor_t*)dbuf;
    DEBUG("usb: EP0 MPS reported as %d\n", ddesc->bMaxPacketSize0);
    (void)ddesc;

    /* TODO(you): for LOW/FULL speed devices issue an Evaluate Context
     * command on EP0 here to update the slot's EP0 MPS to the device-
     * reported value. HIGH/SUPER are always 64/512 so the default is
     * already right. */

    /* Now the full device descriptor. */
    if (usb_get_descriptor(dev, USB_DESC_DEVICE, 0, 0, dbuf,
                           sizeof(usb_device_descriptor_t)) < 0) {
        ERROR("usb: GET_DESCRIPTOR(DEVICE, full) failed\n");
        return;
    }
    dev->dev_desc = *(usb_device_descriptor_t*)dbuf;
    INFO("usb: VID=0x%x PID=0x%x class=0x%x numCfg=%d\n",
         dev->dev_desc.idVendor, dev->dev_desc.idProduct,
         dev->dev_desc.bDeviceClass, dev->dev_desc.bNumConfigurations);

    /* Config descriptor header, then full descriptor. */
    if (usb_get_descriptor(dev, USB_DESC_CONFIGURATION, 0, 0, dbuf,
                           sizeof(usb_config_descriptor_t)) < 0) {
        ERROR("usb: GET_DESCRIPTOR(CONFIG, hdr) failed\n");
        return;
    }
    u16 total = ((usb_config_descriptor_t*)dbuf)->wTotalLength;
    if (total > sizeof(dbuf)) {
        WARNING("usb: config descriptor %d bytes, truncating to %d\n",
                total, (int)sizeof(dbuf));
        total = sizeof(dbuf);
    }
    if (usb_get_descriptor(dev, USB_DESC_CONFIGURATION, 0, 0, dbuf, total) < 0) {
        ERROR("usb: GET_DESCRIPTOR(CONFIG, full) failed\n");
        return;
    }

    if (!parse_config_for_hid(dbuf, total,
                              &dev->config_value,
                              &dev->interface_number,
                              &dev->interface_protocol,
                              &dev->ep_in_addr,
                              &dev->ep_in_mps,
                              &dev->ep_in_interval)) {
        INFO("usb: device on port %d is not a boot-HID; ignoring\n", port);
        return;
    }

    if (usb_set_configuration(dev, dev->config_value) < 0) {
        ERROR("usb: SET_CONFIGURATION(%d) failed\n", dev->config_value);
        return;
    }

    usb_endpoint_descriptor_t epd = {
        .bLength          = sizeof(usb_endpoint_descriptor_t),
        .bDescriptorType  = USB_DESC_ENDPOINT,
        .bEndpointAddress = dev->ep_in_addr,
        .bmAttributes     = USB_ENDPOINT_XFER_INT,
        .wMaxPacketSize   = dev->ep_in_mps,
        .bInterval        = dev->ep_in_interval,
    };
    if (xhci_configure_endpoint(slot, &epd) < 0) {
        ERROR("usb: configure_endpoint failed\n");
        return;
    }

    if (hid_set_protocol(dev, dev->interface_number, HID_PROTOCOL_BOOT) < 0) {
        WARNING("usb: SET_PROTOCOL(BOOT) failed (some devices stall)\n");
    }
    (void)hid_set_idle(dev, dev->interface_number, 0);

    usb_device_attached(dev);
}


/* Called from xhci_handle_event for port-status-change TRBs, and once
 * per port at the end of usb_init for devices that were already attached. */
void usb_port_change(u32 port) {
    volatile u32* portsc = portsc_ptr(port);
    u32 v = *portsc;

    /* Clear change bits if any (W1C). */
    if (v & (XHCI_PORTSC_CSC|XHCI_PORTSC_PEC|XHCI_PORTSC_PRC|XHCI_PORTSC_WRC)) {
        *portsc = (v & ~(XHCI_PORTSC_CSC|XHCI_PORTSC_PEC
                       | XHCI_PORTSC_PRC|XHCI_PORTSC_WRC))
                | (v & (XHCI_PORTSC_CSC|XHCI_PORTSC_PEC
                       | XHCI_PORTSC_PRC|XHCI_PORTSC_WRC));
        asm volatile("dmb sy");
    }

    if (v & XHCI_PORTSC_CCS) {
        INFO("usb: port %d connected\n", port);
        enumerate_port(port);
    } else if (v & XHCI_PORTSC_CSC) {
        INFO("usb: port %d disconnected\n", port);
        /* TODO(you): find the matching usb_device, call hid_detach +
         * Disable Slot command + free DCBAA entry. */
    }
}


/* ----------------------------------------------------------------------- *
 * Notification hooks invoked by xhci.c.
 * ----------------------------------------------------------------------- */
void usb_device_attached(usb_device_t* dev) {
    INFO("usb: device on port %d ready (proto=%d)\n",
         dev->port, dev->interface_protocol);
    hid_attach(dev);
}

void usb_device_detached(usb_device_t* dev) {
    hid_detach(dev);
}
