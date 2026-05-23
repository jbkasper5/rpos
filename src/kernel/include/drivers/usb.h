#ifndef __DRIVERS_USB_H__
#define __DRIVERS_USB_H__

/*
 * USB core API
 * ============
 *
 * This header exposes only the kernel-facing entry points (init + a tiny
 * event API). The internal mechanics live in xhci.h / xhci.c.
 *
 * Layering, top to bottom:
 *
 *     hid.c    ->  parses boot-protocol keyboard / mouse reports
 *     usb.c    ->  enumeration: reset port, GET_DESCRIPTOR, SET_ADDRESS,
 *                  parse config, SET_CONFIGURATION, find HID interface,
 *                  set boot protocol, queue periodic IN transfers
 *     xhci.c   ->  TRB rings, command/event handling, DCBAA, port mgmt
 *     pcie.c   ->  RC bring-up, BAR programming, IRQ unmasking
 *
 * Each layer talks only to the one immediately below it.
 */

#include "macros.h"

/* ---- Standard USB constants --------------------------------------------- */

/* bmRequestType bit fields */
#define USB_DIR_OUT              0x00
#define USB_DIR_IN               0x80
#define USB_TYPE_STANDARD        0x00
#define USB_TYPE_CLASS           0x20
#define USB_TYPE_VENDOR          0x40
#define USB_RECIP_DEVICE         0x00
#define USB_RECIP_INTERFACE      0x01
#define USB_RECIP_ENDPOINT       0x02

/* Standard request codes */
#define USB_REQ_GET_STATUS         0x00
#define USB_REQ_CLEAR_FEATURE      0x01
#define USB_REQ_SET_FEATURE        0x03
#define USB_REQ_SET_ADDRESS        0x05
#define USB_REQ_GET_DESCRIPTOR     0x06
#define USB_REQ_SET_DESCRIPTOR     0x07
#define USB_REQ_GET_CONFIGURATION  0x08
#define USB_REQ_SET_CONFIGURATION  0x09
#define USB_REQ_GET_INTERFACE      0x0A
#define USB_REQ_SET_INTERFACE      0x0B

/* HID class requests */
#define HID_REQ_GET_REPORT        0x01
#define HID_REQ_GET_IDLE          0x02
#define HID_REQ_GET_PROTOCOL      0x03
#define HID_REQ_SET_REPORT        0x09
#define HID_REQ_SET_IDLE          0x0A
#define HID_REQ_SET_PROTOCOL      0x0B

#define HID_PROTOCOL_BOOT         0
#define HID_PROTOCOL_REPORT       1

/* Descriptor types */
#define USB_DESC_DEVICE             0x01
#define USB_DESC_CONFIGURATION      0x02
#define USB_DESC_STRING             0x03
#define USB_DESC_INTERFACE          0x04
#define USB_DESC_ENDPOINT           0x05
#define USB_DESC_HID                0x21
#define USB_DESC_HID_REPORT         0x22

/* Class codes */
#define USB_CLASS_HID               0x03
#define USB_SUBCLASS_BOOT           0x01
#define USB_HID_PROTO_KEYBOARD      0x01
#define USB_HID_PROTO_MOUSE         0x02

/* Endpoint address bits */
#define USB_ENDPOINT_NUMBER_MASK    0x0F
#define USB_ENDPOINT_DIR_MASK       0x80
#define USB_ENDPOINT_XFER_CONTROL   0x00
#define USB_ENDPOINT_XFER_ISO       0x01
#define USB_ENDPOINT_XFER_BULK      0x02
#define USB_ENDPOINT_XFER_INT       0x03
#define USB_ENDPOINT_XFERTYPE_MASK  0x03


/* ---- USB descriptors (packed wire format) ------------------------------- */

typedef struct PACKED {
    u8  bmRequestType;
    u8  bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
} usb_setup_packet_t;

typedef struct PACKED {
    u8  bLength;
    u8  bDescriptorType;
    u16 bcdUSB;
    u8  bDeviceClass;
    u8  bDeviceSubClass;
    u8  bDeviceProtocol;
    u8  bMaxPacketSize0;
    u16 idVendor;
    u16 idProduct;
    u16 bcdDevice;
    u8  iManufacturer;
    u8  iProduct;
    u8  iSerialNumber;
    u8  bNumConfigurations;
} usb_device_descriptor_t;

typedef struct PACKED {
    u8  bLength;
    u8  bDescriptorType;
    u16 wTotalLength;
    u8  bNumInterfaces;
    u8  bConfigurationValue;
    u8  iConfiguration;
    u8  bmAttributes;
    u8  bMaxPower;
} usb_config_descriptor_t;

typedef struct PACKED {
    u8  bLength;
    u8  bDescriptorType;
    u8  bInterfaceNumber;
    u8  bAlternateSetting;
    u8  bNumEndpoints;
    u8  bInterfaceClass;
    u8  bInterfaceSubClass;
    u8  bInterfaceProtocol;
    u8  iInterface;
} usb_interface_descriptor_t;

typedef struct PACKED {
    u8  bLength;
    u8  bDescriptorType;
    u8  bEndpointAddress;
    u8  bmAttributes;
    u16 wMaxPacketSize;
    u8  bInterval;
} usb_endpoint_descriptor_t;

typedef struct PACKED {
    u8  bLength;
    u8  bDescriptorType;
    u16 bcdHID;
    u8  bCountryCode;
    u8  bNumDescriptors;
    u8  bClassDescriptorType;
    u16 wReportDescriptorLength;
} usb_hid_descriptor_t;


/* ---- USB device record (opaque to most callers) ------------------------- */

typedef enum {
    USB_SPEED_LOW   = 1,
    USB_SPEED_FULL  = 2,
    USB_SPEED_HIGH  = 3,
    USB_SPEED_SUPER = 4,
} usb_speed_t;

struct xhci_slot;       /* defined in xhci.h */

typedef struct usb_device {
    u8                          slot_id;
    u8                          address;
    u8                          port;
    usb_speed_t                 speed;

    usb_device_descriptor_t     dev_desc;
    u8                          config_value;
    u8                          interface_number;
    u8                          interface_protocol; /* keyboard/mouse */
    u8                          ep_in_addr;          /* e.g. 0x81 */
    u16                         ep_in_mps;
    u8                          ep_in_interval;

    struct xhci_slot*           slot;
} usb_device_t;


/* ---- Public entry points ------------------------------------------------ */

/* Brings up PCIe, xHCI, and starts enumeration of attached devices.
 * Safe to call before irq_enable(); the controller will start firing
 * once IRQs are unmasked at the GIC. */
void usb_init(void);

/* Called by xhci.c whenever a new device finishes enumeration. */
void usb_device_attached(usb_device_t* dev);

/* Called by xhci.c when a previously-attached device disconnects. */
void usb_device_detached(usb_device_t* dev);


#endif
