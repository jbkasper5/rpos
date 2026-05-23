#ifndef __DRIVERS_XHCI_H__
#define __DRIVERS_XHCI_H__

/*
 * xHCI (eXtensible Host Controller Interface) register, TRB, and context
 * definitions for the Pi 4B's VL805 controller.
 * ====================================================================
 *
 * Reference: xHCI 1.1 spec (Intel document 625472). Section numbers in the
 * comments below all refer to that document.
 *
 * High-level model of what an xHCI controller exposes:
 *
 *   - A bank of "Capability Registers" (read-only IDs + parameters).
 *     Tells us how many slots, how many ports, how big the page must be,
 *     where the operational regs start (CAPLENGTH), where the run-time regs
 *     are (RTSOFF), and where the doorbell array is (DBOFF).
 *
 *   - "Operational Registers" - the things you actually poke to start/stop
 *     the controller, point it at the DCBAA, point it at the command ring,
 *     and read/write port state. (sec 5.4)
 *
 *   - "Runtime Registers" - the per-interrupter event-ring and IMOD config.
 *     We use exactly one interrupter (interrupter 0). (sec 5.5)
 *
 *   - "Doorbell array" - one 32-bit register per slot. Writing here is how
 *     we tell the controller "I added a TRB to slot N's endpoint K's
 *     transfer ring, please process it". DB[0] is the host controller
 *     command doorbell. (sec 5.6)
 *
 *   - "Device Context Base Address Array" (DCBAA) - a host-allocated array
 *     of 64-bit pointers, one per slot. DCBAA[0] is reserved for the
 *     Scratchpad Buffer Array. DCBAA[N>0] is the device context for slot N.
 *     The controller reads it; we hand it the physical address. (sec 6.1)
 *
 *   - Three kinds of TRB rings (each ring is just a contiguous array of
 *     16-byte TRBs that we treat as circular via a "Link TRB" at the end):
 *       * Command ring - HC executes commands we post here (sec 4.6).
 *       * Event ring - HC posts events for us to consume here (sec 4.9).
 *       * Transfer ring - one per active endpoint (sec 4.11).
 *
 * The single hardest concept in xHCI is the "Producer Cycle State" / "Cycle
 * Bit". On each ring there's a one-bit owner flag we toggle each time the
 * ring's enqueue pointer wraps. The HC owns TRBs whose Cycle Bit matches the
 * current PCS; we own the rest. The TODO in xhci.c walks through this with
 * a worked example before you implement it.
 */

#include "macros.h"
#include "drivers/usb.h"

/* ---- Capability Registers (offset 0 from MMIO base) --------------------- */

typedef volatile struct PACKED {
    reg32_t caplength_hciversion;   /* 0x00: low byte = CAPLENGTH, high half = HCIVERSION */
    reg32_t hcsparams1;             /* 0x04 */
    reg32_t hcsparams2;             /* 0x08 */
    reg32_t hcsparams3;             /* 0x0C */
    reg32_t hccparams1;             /* 0x10 */
    reg32_t dboff;                  /* 0x14: doorbell offset */
    reg32_t rtsoff;                 /* 0x18: runtime register offset */
    reg32_t hccparams2;             /* 0x1C */
} xhci_cap_regs_t;

/* HCSPARAMS1 fields */
#define XHCI_HCS1_MAXSLOTS(x)      (((x) >>  0) & 0xff)
#define XHCI_HCS1_MAXINTRS(x)      (((x) >>  8) & 0x7ff)
#define XHCI_HCS1_MAXPORTS(x)      (((x) >> 24) & 0xff)

#define XHCI_HCS2_MAX_SCRATCHPAD_HI(x) (((x) >> 21) & 0x1f)
#define XHCI_HCS2_MAX_SCRATCHPAD_LO(x) (((x) >> 27) & 0x1f)

#define XHCI_HCCP1_AC64            (1u << 0)   /* 64-bit addressing? */
#define XHCI_HCCP1_CSZ             (1u << 2)   /* context size: 0=32B, 1=64B */

/* ---- Operational Registers (offset CAPLENGTH from MMIO base) ------------ */

typedef volatile struct PACKED {
    reg32_t usbcmd;                 /* 0x00 */
    reg32_t usbsts;                 /* 0x04 */
    reg32_t pagesize;               /* 0x08 */
    reg32_t reserved0[2];           /* 0x0C-0x13 */
    reg32_t dnctrl;                 /* 0x14 */
    reg64_t crcr;                   /* 0x18: Command Ring Control Register */
    reg32_t reserved1[4];           /* 0x20-0x2F */
    reg64_t dcbaap;                 /* 0x30 */
    reg32_t config;                 /* 0x38 */
    /* Port Register Set (4 regs * MAXPORTS) starts at offset 0x400 */
} xhci_op_regs_t;

#define XHCI_USBCMD_RS             (1u << 0)
#define XHCI_USBCMD_HCRST          (1u << 1)
#define XHCI_USBCMD_INTE           (1u << 2)
#define XHCI_USBCMD_HSEE           (1u << 3)

#define XHCI_USBSTS_HCH            (1u << 0)   /* HC Halted */
#define XHCI_USBSTS_HSE            (1u << 2)   /* Host System Error */
#define XHCI_USBSTS_EINT           (1u << 3)   /* Event Interrupt */
#define XHCI_USBSTS_PCD            (1u << 4)   /* Port Change Detect */
#define XHCI_USBSTS_CNR            (1u << 11)  /* Controller Not Ready */
#define XHCI_USBSTS_HCE            (1u << 12)  /* Host Controller Error */

#define XHCI_CRCR_RCS              (1u << 0)
#define XHCI_CRCR_CS               (1u << 1)
#define XHCI_CRCR_CA               (1u << 2)
#define XHCI_CRCR_CRR              (1u << 3)

#define XHCI_PORTSC_CCS            (1u << 0)   /* Current Connect Status */
#define XHCI_PORTSC_PED            (1u << 1)   /* Port Enabled */
#define XHCI_PORTSC_PR             (1u << 4)   /* Port Reset */
#define XHCI_PORTSC_PLS_SHIFT      5
#define XHCI_PORTSC_PLS_MASK       (0xfu << 5)
#define XHCI_PORTSC_PP             (1u << 9)   /* Port Power */
#define XHCI_PORTSC_SPEED_SHIFT    10
#define XHCI_PORTSC_SPEED_MASK     (0xfu << 10)
#define XHCI_PORTSC_CSC            (1u << 17)  /* Connect Status Change */
#define XHCI_PORTSC_PEC            (1u << 18)  /* Port Enable Change */
#define XHCI_PORTSC_WRC            (1u << 19)  /* Warm Reset Change */
#define XHCI_PORTSC_PRC            (1u << 21)  /* Port Reset Change */

/* The Port Reg Set is (0x400 + 0x10*(port-1)) from op base. */
#define XHCI_PORT_REG_BASE(op_base, port_1based) \
    ((volatile u32*)((u64)(op_base) + 0x400 + 0x10 * ((port_1based) - 1)))


/* ---- Runtime Registers (Interrupter 0) ---------------------------------- */

typedef volatile struct PACKED {
    reg32_t iman;       /* Interrupter Management */
    reg32_t imod;       /* Interrupter Moderation */
    reg32_t erstsz;     /* Event Ring Segment Table Size */
    reg32_t reserved;
    reg64_t erstba;     /* Event Ring Segment Table Base Address */
    reg64_t erdp;       /* Event Ring Dequeue Pointer */
} xhci_intr_regs_t;

#define XHCI_IMAN_IP               (1u << 0)   /* Interrupt Pending - W1C */
#define XHCI_IMAN_IE               (1u << 1)   /* Interrupt Enable */

#define XHCI_ERDP_EHB              (1ull << 3) /* Event Handler Busy - W1C */


/* ---- TRB (Transfer Request Block) - 16 bytes ---------------------------- */

/* TRB types (sec 6.4.6). Stored in Type field of Control dword. */
#define TRB_TYPE_NORMAL                  1
#define TRB_TYPE_SETUP_STAGE             2
#define TRB_TYPE_DATA_STAGE              3
#define TRB_TYPE_STATUS_STAGE            4
#define TRB_TYPE_LINK                    6
#define TRB_TYPE_ENABLE_SLOT             9
#define TRB_TYPE_DISABLE_SLOT           10
#define TRB_TYPE_ADDRESS_DEVICE         11
#define TRB_TYPE_CONFIGURE_ENDPOINT     12
#define TRB_TYPE_EVALUATE_CONTEXT       13
#define TRB_TYPE_RESET_ENDPOINT         14
#define TRB_TYPE_STOP_ENDPOINT          15
#define TRB_TYPE_NOOP_COMMAND           23
#define TRB_TYPE_TRANSFER_EVENT         32
#define TRB_TYPE_COMMAND_COMPLETION     33
#define TRB_TYPE_PORT_STATUS_CHANGE     34
#define TRB_TYPE_HOST_CONTROLLER_EVENT  37

/* Completion codes (sec 6.4.5) - found in event TRBs */
#define TRB_CC_INVALID              0
#define TRB_CC_SUCCESS              1
#define TRB_CC_DATA_BUFFER_ERROR    2
#define TRB_CC_BABBLE_DETECTED      3
#define TRB_CC_USB_TRANSACTION_ERR  4
#define TRB_CC_TRB_ERROR            5
#define TRB_CC_STALL_ERROR          6
#define TRB_CC_RESOURCE_ERROR       7
#define TRB_CC_BANDWIDTH_ERROR      8
#define TRB_CC_NO_SLOTS_AVAILABLE   9
#define TRB_CC_SHORT_PACKET         13

/* TRB control-dword bit layout (cycle in bit 0, type in bits [15:10]). */
#define TRB_CYCLE_BIT               (1u << 0)
#define TRB_TYPE_SHIFT              10
#define TRB_TYPE_MASK               (0x3fu << 10)
#define TRB_GET_TYPE(ctrl)          (((ctrl) >> 10) & 0x3f)
#define TRB_SET_TYPE(t)             ((u32)(t) << 10)

/* Common bits we OR into control */
#define TRB_CTRL_ENT                (1u << 1)   /* Evaluate Next TRB */
#define TRB_CTRL_ISP                (1u << 2)   /* Interrupt on Short Packet */
#define TRB_CTRL_NS                 (1u << 3)
#define TRB_CTRL_CHAIN              (1u << 4)
#define TRB_CTRL_IOC                (1u << 5)   /* Interrupt On Completion */
#define TRB_CTRL_IDT                (1u << 6)   /* Immediate Data (Setup TRB) */
#define TRB_CTRL_BEI                (1u << 9)   /* Block Event Interrupt */
#define TRB_CTRL_TOGGLE_CYCLE       (1u << 1)   /* Link-TRB only */

/* Generic 16-byte TRB. Specific TRB variants reinterpret these fields. */
typedef struct PACKED {
    u32 parameter_lo;       /* often a DMA pointer's low 32 bits */
    u32 parameter_hi;
    u32 status;             /* transfer length + completion code */
    u32 control;            /* cycle + type + flags */
} xhci_trb_t;
_Static_assert(sizeof(xhci_trb_t) == 16, "TRB must be 16 bytes");


/* ---- Event Ring Segment Table entry ------------------------------------- */

typedef struct PACKED {
    u64 ring_segment_base;
    u32 ring_segment_size;
    u32 reserved;
} xhci_erst_entry_t;
_Static_assert(sizeof(xhci_erst_entry_t) == 16, "ERST entry must be 16 bytes");


/* ---- Device / Input contexts (sec 6.2) ---------------------------------- *
 *
 * The controller can be configured for either 32-byte or 64-byte contexts
 * depending on HCCPARAMS1.CSZ. The VL805 reports CSZ = 0 (32-byte). We
 * support only the 32-byte form here.
 *
 * An Input Context is { Input Control Context, Slot Context, EP Contexts[1..31] }
 * A Device Context is { Slot Context, EP Contexts[1..31] }
 *
 * EP index 1 in these arrays is the control endpoint EP0 (which has a
 * single ring shared by IN and OUT). EP index 2 onwards corresponds to
 *   DCI = 2*N + (dir == IN ? 1 : 0)
 * where N is the endpoint number from the descriptor.
 */

#define XHCI_CTX_SIZE_BYTES         32
#define XHCI_EP_COUNT               32          /* DCI range 0..31 */

typedef struct PACKED {
    u32 drop_flags;
    u32 add_flags;
    u32 reserved[6];
} xhci_input_ctrl_ctx_t;

typedef struct PACKED {
    u32 route_string_speed;     /* RouteString[19:0], Speed[23:20], etc */
    u32 max_exit_latency_port;  /* RootHubPort[23:16], NumberOfPorts[31:24] */
    u32 tt_info;
    u32 device_address_state;   /* USB Address[7:0], Slot State[31:27] */
    u32 reserved[4];
} xhci_slot_ctx_t;

typedef struct PACKED {
    u32 ep_info1;               /* EP State[2:0], Interval[23:16], MaxPStreams, LSA, ... */
    u32 ep_info2;               /* Mult[1:0], MaxPStreams[4:0], EP Type[5:3], CErr[2:1], HID[7], MaxBurst[15:8], MPS[31:16] */
    u64 tr_dequeue_ptr;         /* low bit = DCS (dequeue cycle state) */
    u32 tx_info;                /* AverageTRBLen[15:0], MaxESITPayload[31:16] */
    u32 reserved[3];
} xhci_ep_ctx_t;

/* Endpoint types stored in ep_info2[5:3] */
#define XHCI_EPTYPE_ISOCH_OUT       1
#define XHCI_EPTYPE_BULK_OUT        2
#define XHCI_EPTYPE_INTERRUPT_OUT   3
#define XHCI_EPTYPE_CONTROL         4
#define XHCI_EPTYPE_ISOCH_IN        5
#define XHCI_EPTYPE_BULK_IN         6
#define XHCI_EPTYPE_INTERRUPT_IN    7


/* ---- Ring abstraction (host-side bookkeeping) --------------------------- *
 *
 * The host driver maintains these alongside the actual ring memory the
 * controller sees. A ring is a fixed-size array of TRBs whose last TRB is
 * a Link TRB pointing to the start, with the LINK's "Toggle Cycle" bit set
 * so the producer cycle state flips on wrap.
 */

typedef struct xhci_ring {
    xhci_trb_t* trbs;           /* virtual addr of TRB array */
    u64         trbs_phys;      /* physical addr (for HC) */
    u32         num_trbs;       /* including the link TRB at end */
    u32         enqueue_idx;    /* producer index, owned by us */
    u32         dequeue_idx;    /* consumer index (for event ring) */
    u8          cycle_state;    /* current PCS (producer) or CCS (consumer) */
} xhci_ring_t;


/* ---- Per-slot bookkeeping ----------------------------------------------- */

typedef struct xhci_slot {
    u8                  slot_id;
    bool                in_use;
    void*               device_ctx;     /* virtual; physical stored in DCBAA */
    void*               input_ctx;      /* virtual */
    xhci_ring_t         ep_rings[XHCI_EP_COUNT];   /* index by DCI; only some populated */
    struct usb_device*  usb_dev;
} xhci_slot_t;


/* ---- Public xHCI API (used by usb.c) ------------------------------------ */

bool xhci_init(u64 mmio_virt, u32 mmio_size);

/* Submit a control transfer (used by enumeration). Blocks until completion
 * event arrives. data is a virtual address; can be NULL if wLength==0. */
int  xhci_control_transfer(xhci_slot_t* slot,
                           usb_setup_packet_t* setup,
                           void* data);

/* Submit a single interrupt-IN TRB onto an endpoint's transfer ring and
 * ring the doorbell. Completion arrives asynchronously via the IRQ path. */
int  xhci_queue_interrupt_in(xhci_slot_t* slot, u8 dci,
                             void* buf, u32 len);

/* Allocate a slot via Enable Slot command. Returns slot ptr or NULL. */
xhci_slot_t* xhci_enable_slot(void);

/* Address Device command. After this, the slot has a USB address and EP0
 * is configured; control transfers will work. Implementation: TODO (yours). */
int  xhci_address_device(xhci_slot_t* slot, u8 port, usb_speed_t speed);

/* Configure (or re-configure) a slot's endpoints from a parsed descriptor
 * set. Implementation: TODO (yours). */
int  xhci_configure_endpoint(xhci_slot_t* slot,
                             usb_endpoint_descriptor_t* ep_desc);

/* Called by the IRQ dispatcher. Drains the event ring. */
void xhci_handle_irq(void);

/* Diagnostic dump (USBSTS, PORTSC, etc). */
void xhci_dump_state(void);


/* ---- DMA allocator (xHCI requires alignment + low-physical-address) ----- *
 *
 * Your kmalloc lacks alignment control AND has no concept of "DMA-safe"
 * physical addresses. This helper bridges the gap.
 *
 * Returns a virtual address of `size` bytes aligned to `align`, whose
 * physical address is < PCIE_DMA_MASK (so the VL805 can reach it). Memory
 * is zeroed and cleaned out of the dcache so the device sees correct data.
 *
 * This is a TODO in xhci.c (see xhci_dma_alloc) - we have implemented a
 * naive version that should be replaced with a proper dma_alloc_coherent
 * once you've thought about cache strategy. See comments there.
 */
void* xhci_dma_alloc(u32 size, u32 align);

/* Translate kernel virtual -> physical (= bus address, since we set up a
 * 1:1 inbound PCIe window). Wrapper over va_to_pa with a sanity check. */
u64   xhci_va_to_dma(void* va);

#endif
