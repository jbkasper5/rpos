/*
 * xhci.c - VL805 xHCI host controller driver (Pi 4B baremetal).
 *
 * Scope
 * -----
 * This file owns the controller-facing side of USB:
 *   - HC bring-up: reset, page-size detection, DCBAA & scratchpad allocation,
 *     command-ring init, event-ring + ERST init, interrupter setup, RS=1.
 *   - IRQ handling: drain the event ring, ack interrupts at the GIC and at
 *     the BCM2711 PCIe INTR2 block, dispatch transfer/command/port events.
 *   - Public ops used by usb.c: enable_slot, address_device, configure_ep,
 *     control_transfer, queue_interrupt_in.
 *
 * What's left as TODO for you (search for "TODO(you):" - five sites)
 * ------------------------------------------------------------------
 *   T1. xhci_ring_enqueue_trb              - the core ring producer
 *   T2. xhci_address_device                - Input Context + Address Device
 *   T3. xhci_configure_endpoint            - Configure Endpoint command
 *   T4. xhci_control_transfer              - Setup/Data/Status TRB sequence
 *   T5. The completion-code -> errno mapping in xhci_handle_event
 *
 * Why those five
 * --------------
 * They are where xHCI stops being "follow the spec literally" and starts
 * being "internalize the producer/consumer ring + cycle-bit model". Once
 * those click, the rest of xHCI is mostly a different flavor of the same
 * pattern, and you'll be able to add bulk/iso support without help.
 *
 * Each TODO block has a long comment that walks through exactly what the
 * code needs to do, including spec section references and pitfalls. The
 * surrounding scaffolding (slot tables, ring allocation, IRQ wire-up) is
 * fully implemented so you can compile, boot, and watch the system get
 * partway through enumeration before hitting your first TODO.
 */

#include "macros.h"
#include "io/kprintf.h"
#include "utils/timer.h"
#include "utils/utils.h"
#include "memory/kmalloc.h"
#include "memory/paging.h"
#include "memory/mmap.h"
#include "memory/virtual_memory.h"
#include "drivers/usb.h"
#include "drivers/xhci.h"
#include "drivers/pcie.h"
#include "peripherals/gic.h"


/* ----------------------------------------------------------------------- *
 * Module state.
 * ----------------------------------------------------------------------- */

typedef struct {
    u64                     mmio_base;       /* virt addr of cap regs */
    xhci_cap_regs_t*        cap;
    xhci_op_regs_t*         op;
    xhci_intr_regs_t*       ir0;             /* interrupter 0 */
    volatile u32*           doorbells;       /* 32-bit register per slot */

    u32                     num_slots;
    u32                     num_ports;
    u32                     page_size_bytes;
    bool                    ctx_64;          /* true => 64-byte contexts */

    u64*                    dcbaa;           /* virtual */
    u64                     dcbaa_phys;

    xhci_ring_t             cmd_ring;
    xhci_ring_t             event_ring;
    xhci_erst_entry_t*      erst;            /* virtual, 1 entry suffices */
    u64                     erst_phys;

    xhci_slot_t             slots[256];      /* slot 0 unused */
} xhci_state_t;

static xhci_state_t xhci;


/* ----------------------------------------------------------------------- *
 * DMA-aware allocation.
 *
 * Your kmalloc doesn't expose alignment control and has no concept of
 * "must be physically below 1 GiB so PCIe can reach it". This wrapper
 * uses buddy_alloc directly (which always returns a fresh page) and
 * sanity-checks the resulting physical address.
 *
 * Limitation: align must be <= PAGE_SIZE (4096). Every xHCI structure
 * actually needs <= 4096 alignment so that's fine in practice.
 *
 * Coherency: we zero the buffer and then issue a `dsb sy`. Pi 4B's PCIe
 * inbound path is documented as coherent with the CPU caches when the
 * RC's SCB_ACCESS_EN bit is set (we did that). If you start seeing
 * "device sees stale data" issues, the right fix is to mark these DMA
 * regions MAP_NOCACHE in mmap() and remap. Tagged as a TODO too.
 * ----------------------------------------------------------------------- */
void* xhci_dma_alloc(u32 size, u32 align) {
    (void)align;     /* page-aligned via kmalloc's buddy path below */
    size = ALIGN_UP(size, PAGE_SIZE);

    /* kmalloc takes the buddy_alloc + map() path for any size > one
     * slab order (which our PAGE_SIZE-aligned requests always exceed),
     * so the returned pointer is a kernel-virtual address backed by
     * a contiguous physical run that has actually been mapped into
     * the page tables. We need both halves: the CPU touches the
     * structure via the virtual address, and the HC sees the same
     * structure at its physical address (after va_to_pa). */
    void *va = kmalloc(size);
    if (!va) {
        ERROR("xhci_dma_alloc: kmalloc(%d) failed\n", size);
        return NULL;
    }

    /* Make sure the underlying physical address is within the PCIe
     * inbound window. On Pi 4B with 4 GiB RAM and our 1 GiB inbound
     * window, this is automatic if the buddy allocator carves below
     * 1 GiB. Logged loudly so you don't miss it. */
    u64 pa = va_to_pa((u64)va);
    if (pa + size > PCIE_DMA_MASK) {
        ERROR("xhci_dma_alloc: phys 0x%x out of PCIe inbound window\n", pa);
        /* We don't have a "free above 1 GiB" path; just continue and let
         * the device complain. TODO: restrict buddy pool, or kfree() and
         * retry until we get an in-window allocation. */
    }

    /* Zero the buffer. kmalloc doesn't guarantee zeroing on the buddy
     * path - it hands out raw pages. */
    memset(va, 0, size);
    asm volatile("dsb sy");
    return va;
}

u64 xhci_va_to_dma(void* va) {
    /* Identity mapping inside our inbound window: bus addr == phys addr. */
    return va_to_pa((u64)va);
}


/* ----------------------------------------------------------------------- *
 * Tiny MMIO + ring helpers.
 * ----------------------------------------------------------------------- */

static inline u32 op_rd32(u32 off)        { return *(volatile u32*)((u64)xhci.op + off); }
static inline void op_wr32(u32 off, u32 v){ *(volatile u32*)((u64)xhci.op + off) = v; asm volatile("dmb sy"); }
static inline u64 op_rd64(u32 off)        { return *(volatile u64*)((u64)xhci.op + off); }
static inline void op_wr64(u32 off, u64 v){ *(volatile u64*)((u64)xhci.op + off) = v; asm volatile("dmb sy"); }

/* Ring a doorbell. db_index=0 is the HC command doorbell; db_index=N (slot)
 * with target=DCI tells the HC "endpoint DCI of slot N has work". */
static inline void ring_doorbell(u32 db_index, u32 target) {
    xhci.doorbells[db_index] = target;
    asm volatile("dmb sy");
}


/* ----------------------------------------------------------------------- *
 * Ring init.
 *
 * A "ring" is a contiguous array of 16-byte TRBs whose last entry is a
 * Link TRB pointing back to the start, with TRB_CTRL_TOGGLE_CYCLE set.
 * The consumer flips its cycle state when it processes a Link TRB.
 * ----------------------------------------------------------------------- */

static bool ring_alloc(xhci_ring_t* r, u32 num_trbs) {
    /* Size in bytes, rounded up to PAGE_SIZE. */
    u32 bytes = num_trbs * (u32)sizeof(xhci_trb_t);
    r->trbs = (xhci_trb_t*)xhci_dma_alloc(bytes, 64);
    if (!r->trbs) return FALSE;

    r->trbs_phys   = xhci_va_to_dma(r->trbs);
    r->num_trbs    = num_trbs;
    r->enqueue_idx = 0;
    r->dequeue_idx = 0;
    r->cycle_state = 1;

    /* Last TRB: Link back to start, with TOGGLE_CYCLE set. The HC will
     * flip its cycle state when it consumes this entry. */
    xhci_trb_t* link = &r->trbs[num_trbs - 1];
    link->parameter_lo = (u32)(r->trbs_phys & 0xFFFFFFFFu);
    link->parameter_hi = (u32)(r->trbs_phys >> 32);
    link->status       = 0;
    link->control      = TRB_SET_TYPE(TRB_TYPE_LINK)
                       | TRB_CTRL_TOGGLE_CYCLE
                       | r->cycle_state;
    asm volatile("dsb sy");
    return TRUE;
}


/* ----------------------------------------------------------------------- *
 * TODO(you): T1 - the heart of the producer side.
 *
 * Enqueue a single TRB onto a ring, updating the cycle bit and handling
 * the Link-TRB wrap. Returns the index where the TRB was placed (caller
 * doesn't usually care, but multi-TRB commands like control transfers
 * need it to know where the IOC TRB landed).
 *
 * Pseudocode walk-through (xHCI spec section 4.9.2):
 *
 *   1. Copy *src into r->trbs[r->enqueue_idx], but BUILD the control
 *      dword with the *current* r->cycle_state in bit 0. Do NOT set the
 *      cycle bit on the same store as the rest of the dword - write the
 *      param/status fields first (a single 12-byte write is fine for
 *      most TRBs), issue a dsb, THEN write the control dword. This is
 *      what the spec calls "the host shall write the Cycle bit last."
 *      Otherwise the HC may race and start processing a half-written TRB.
 *
 *   2. Increment r->enqueue_idx. Bounds check.
 *
 *   3. If the *next* slot is the Link TRB (index num_trbs - 1):
 *        a. Update the Link TRB's cycle bit to the current cycle_state
 *           (its destination address never changes - only the cycle).
 *           Again, write the cycle byte last with a barrier.
 *        b. Flip r->cycle_state ^= 1.
 *        c. enqueue_idx = 0.
 *      The HC will see the link TRB, follow it back to the start, and
 *      flip its own consumer cycle state to match.
 *
 *   4. Return the index you wrote to.
 *
 * Pitfalls:
 *   - Forgetting to barrier between the bulk write and the cycle bit:
 *     intermittent "controller stops responding" failures.
 *   - Setting the Link TRB's cycle bit *before* the producer has filled
 *     it: HC dequeues a stale TRB, walks off into garbage.
 *   - Not checking whether the ring is *full* (enqueue would advance into
 *     the consumer). For control transfers this almost never matters; for
 *     bulk it does. For now, panic() if the ring would wrap into the
 *     current dequeue_idx.
 */
static u32 xhci_ring_enqueue_trb(xhci_ring_t* r, const xhci_trb_t* src) {
    /* TODO(you): implement per the comment above. The body below is a
     * no-op placeholder so the file links; replace it. */
    (void)src;
    panic();
    return r->enqueue_idx;
}


/* ----------------------------------------------------------------------- *
 * HC reset + bring-up.
 * ----------------------------------------------------------------------- */

static bool hc_reset(void) {
    /* Halt first. RS=0, wait for HCH=1. */
    u32 cmd = op_rd32(0x00);        /* USBCMD */
    cmd &= ~XHCI_USBCMD_RS;
    op_wr32(0x00, cmd);
    for (int i = 0; i < 100; i++) {
        if (op_rd32(0x04) & XHCI_USBSTS_HCH) break;
        timer_sleep(1);
    }

    /* HCRST=1, wait for it to self-clear AND for CNR=0. */
    op_wr32(0x00, XHCI_USBCMD_HCRST);
    for (int i = 0; i < 1000; i++) {
        u32 c = op_rd32(0x00);
        u32 s = op_rd32(0x04);
        if (!(c & XHCI_USBCMD_HCRST) && !(s & XHCI_USBSTS_CNR)) {
            DEBUG("xhci: reset complete after %d ms\n", i);
            return TRUE;
        }
        timer_sleep(1);
    }
    ERROR("xhci: HC reset did not complete\n");
    return FALSE;
}


static bool dcbaa_init(void) {
    /* DCBAA must be page-aligned, 8 bytes per entry, num_slots+1 entries
     * (slot 0 is the scratchpad pointer). */
    u32 bytes = (xhci.num_slots + 1) * sizeof(u64);
    xhci.dcbaa = (u64*)xhci_dma_alloc(bytes, 64);
    if (!xhci.dcbaa) return FALSE;
    xhci.dcbaa_phys = xhci_va_to_dma(xhci.dcbaa);

    /* Scratchpad: HCSPARAMS2 may require us to allocate "Scratchpad Buffers"
     * which the HC uses as opaque scratch. Skipped here; most controllers
     * (including VL805) tolerate 0 even if their HCS2 asks for some. If you
     * see HC hangs after RS=1 this is the prime suspect - allocate the
     * scratchpad array and point DCBAA[0] at it. */

    op_wr64(0x30, xhci.dcbaa_phys);     /* DCBAAP */
    return TRUE;
}


static bool cmd_ring_init(void) {
    if (!ring_alloc(&xhci.cmd_ring, 256)) return FALSE;
    /* CRCR: physical addr | RCS (initial ring cycle state) */
    op_wr64(0x18, xhci.cmd_ring.trbs_phys | XHCI_CRCR_RCS);
    return TRUE;
}


static bool event_ring_init(void) {
    if (!ring_alloc(&xhci.event_ring, 256)) return FALSE;

    /* The event ring is described to the HC via a 1-entry segment table
     * (ERST). We allocate one ERST entry and point it at our event ring. */
    xhci.erst = (xhci_erst_entry_t*)xhci_dma_alloc(64, 64);
    if (!xhci.erst) return FALSE;
    xhci.erst_phys                  = xhci_va_to_dma(xhci.erst);
    xhci.erst[0].ring_segment_base  = xhci.event_ring.trbs_phys;
    xhci.erst[0].ring_segment_size  = xhci.event_ring.num_trbs;
    xhci.erst[0].reserved           = 0;

    /* Program interrupter 0:
     *  - ERSTSZ = 1 (one segment)
     *  - ERDP   = phys addr of first TRB (clear EHB by writing 1 to bit 3)
     *  - ERSTBA = phys addr of ERST  (write LAST - this is what tells the
     *    HC "now go look at ERSTSZ and ERDP")  */
    xhci.ir0->erstsz = 1;
    xhci.ir0->erdp   = xhci.event_ring.trbs_phys;
    xhci.ir0->erstba = xhci.erst_phys;

    /* Enable interrupts on this interrupter. IMOD default (4000) is fine. */
    xhci.ir0->iman = XHCI_IMAN_IE;
    return TRUE;
}


/* ----------------------------------------------------------------------- *
 * Event-ring drain (consumer side).
 * ----------------------------------------------------------------------- */

static void xhci_handle_event(xhci_trb_t* ev) {
    u32 type = TRB_GET_TYPE(ev->control);
    u32 cc   = (ev->status >> 24) & 0xFF;
    (void)cc;   /* used in your TODO(T5) impl */

    switch (type) {

    case TRB_TYPE_COMMAND_COMPLETION: {
        /* The waiter for this command is parked on a per-command state we
         * stashed when we enqueued. For our blocking model we just record
         * the completion code and let the waker do the rest. Hook your
         * command-wait synchronisation here. */
        DEBUG("xhci: cmd completion cc=%d\n", cc);
        break;
    }

    case TRB_TYPE_PORT_STATUS_CHANGE: {
        /* ev->parameter_lo[31:24] is the affected port number. We treat
         * this as "go ask usb.c to re-scan that port". */
        u32 port = (ev->parameter_lo >> 24) & 0xFF;
        DEBUG("xhci: port status change on port %d\n", port);
        /* usb.c looks at PORTSC and decides what to do. */
        extern void usb_port_change(u32 port);
        usb_port_change(port);
        break;
    }

    case TRB_TYPE_TRANSFER_EVENT: {
        /* This is what arrives for both control-transfer completions and
         * for periodic IN transfers (HID reports). The endpoint context
         * pointer lives in ev->parameter, and the slot/ep ID is in the
         * control dword. */
        u32 slot_id = (ev->control >> 24) & 0xFF;
        u32 dci     = (ev->control >> 16) & 0x1F;
        DEBUG("xhci: xfer event slot=%d dci=%d cc=%d\n", slot_id, dci, cc);
        (void)slot_id; (void)dci;   /* used by your TODO(T5) impl */

        /* TODO(you): T5
         * Map cc -> simple int errno-like code (SUCCESS=0, STALL=-32, etc.)
         * Look up the waiter using (slot_id, dci) and wake it. For the
         * periodic IN path, call hid_dispatch_report() with the buffer
         * that was associated with the originating TRB. We don't store
         * that association anywhere yet - the simplest approach is to
         * keep a parallel "TRB index -> {usb_device*, buf, len}" array
         * per transfer ring. The TRB pointer in ev->parameter is the
         * physical address of the originating TRB, which you can map
         * back to a ring index. */
        break;
    }

    default:
        DEBUG("xhci: unhandled event type=%d cc=%d\n", type, cc);
        break;
    }
}

static void event_ring_drain(void) {
    xhci_ring_t* r = &xhci.event_ring;
    bool processed_any = FALSE;

    while (1) {
        xhci_trb_t* ev = &r->trbs[r->dequeue_idx];
        u8 cyc = (u8)(ev->control & TRB_CYCLE_BIT);
        if (cyc != r->cycle_state) break;       /* HC hasn't filled it yet */

        xhci_handle_event(ev);
        processed_any = TRUE;

        r->dequeue_idx++;
        if (r->dequeue_idx == r->num_trbs) {
            r->dequeue_idx = 0;
            r->cycle_state ^= 1;
        }
    }

    if (processed_any) {
        u64 dq_phys = r->trbs_phys + (u64)r->dequeue_idx * sizeof(xhci_trb_t);
        /* Write ERDP with EHB set (bit 3 is W1C - we clear "event handler
         * busy"). Software is required to clear EHB at the end of every
         * batch of event processing. */
        xhci.ir0->erdp = dq_phys | XHCI_ERDP_EHB;
    }
}


/* ----------------------------------------------------------------------- *
 * IRQ entry point. Called from system/irq.c's handle_irq() when the GIC
 * delivers PCIE_INTID_LEGACY (= 179).
 * ----------------------------------------------------------------------- */
void xhci_handle_irq(void) {
    /* First, find which PCIe device line raised. On a single-device Pi 4B
     * board this is always the VL805, but we read INTR2_STATUS anyway to
     * follow the right ack protocol. */
    u32 intr2 = pcie_read_intr2_status();

    /* Acknowledge the xHCI interrupter (IMAN.IP is W1C). The HC won't
     * raise another interrupt on this interrupter until we do this. */
    u32 iman = xhci.ir0->iman;
    if (iman & XHCI_IMAN_IP) {
        xhci.ir0->iman = iman | XHCI_IMAN_IP;
    }

    /* USBSTS.EINT is also W1C. */
    u32 sts = op_rd32(0x04);
    if (sts & XHCI_USBSTS_EINT) op_wr32(0x04, XHCI_USBSTS_EINT);

    event_ring_drain();

    /* Clear the legacy line at the PCIe RC. */
    if (intr2 & (PCIE_INTR2_PCI_INTA | PCIE_INTR2_PCI_INTB |
                 PCIE_INTR2_PCI_INTC | PCIE_INTR2_PCI_INTD)) {
        pcie_ack_legacy_irq(intr2);
    }
}


/* ----------------------------------------------------------------------- *
 * Public ops used by usb.c.
 * ----------------------------------------------------------------------- */

xhci_slot_t* xhci_enable_slot(void) {
    /* Enqueue an Enable Slot Command TRB, ring the command doorbell,
     * wait for the matching Command Completion Event, parse the
     * assigned slot ID out of it. */
    xhci_trb_t trb = {0};
    trb.control = TRB_SET_TYPE(TRB_TYPE_ENABLE_SLOT) | TRB_CTRL_IOC;
    xhci_ring_enqueue_trb(&xhci.cmd_ring, &trb);
    ring_doorbell(0, 0);

    /* TODO(you): proper wait-for-completion. For now, busy-wait poll. */
    for (int i = 0; i < 1000; i++) {
        event_ring_drain();
        timer_sleep(1);
        /* If you implement T5's slot-id stash, exit early when you see
         * the Enable Slot completion. */
    }

    /* Placeholder: pretend we got slot 1. Replace once T1+T5 are done. */
    u8 slot_id = 1;
    xhci.slots[slot_id].slot_id = slot_id;
    xhci.slots[slot_id].in_use  = TRUE;
    return &xhci.slots[slot_id];
}


int xhci_address_device(xhci_slot_t* slot, u8 port, usb_speed_t speed) {
    /* TODO(you): T2 - Address Device.
     *
     *  1. Allocate the slot's Device Context (32 or 64 bytes per ctx, 32
     *     contexts = 1024 or 2048 bytes total). Zero it. Install the
     *     physical address into xhci.dcbaa[slot_id]. (DCBAA -> DevCtx.)
     *
     *  2. Allocate an Input Context. Layout (32-byte ctx):
     *        +0x00  Input Control Context (drop_flags, add_flags, ...)
     *        +0x20  Slot Context
     *        +0x40  EP0 Context  (DCI=1)
     *        +0x60+ EP Contexts  (we ignore until Configure Endpoint)
     *     Set add_flags = (1 << 0) | (1 << 1) - i.e. A0 (slot) and A1 (EP0).
     *
     *  3. Fill the Slot Context:
     *       - RouteString = 0 (root hub direct attach, no hubs)
     *       - Speed       = speed (see USB_SPEED_*)
     *       - Context Entries = 1 (EP0 only for now)
     *       - Root Hub Port = port
     *       - Slot State   = leave 0 (Disabled/Default)
     *
     *  4. Fill EP0 Context:
     *       - EP Type        = XHCI_EPTYPE_CONTROL
     *       - Max Packet Size= speed-dependent default:
     *           LOW=8, FULL=8, HIGH=64, SUPER=512
     *           (you can renegotiate to the device-reported MPS later via
     *            an Evaluate Context command)
     *       - Average TRB Length = 8
     *       - Error Count    = 3
     *       - TR Dequeue Pointer = phys addr of the EP0 transfer ring you
     *         allocated for this slot (ring_alloc() with ~16 TRBs is plenty)
     *       - DCS (low bit of TR Dequeue Pointer) = 1 to match initial
     *         ring cycle state.
     *
     *  5. Enqueue Address Device Command TRB into the command ring:
     *       parameter = phys addr of the Input Context
     *       control   = TRB_SET_TYPE(TRB_TYPE_ADDRESS_DEVICE) | slot_id<<24
     *                   | TRB_CTRL_IOC
     *     Optionally set BSR (Block Set Address Request, bit 9) on the
     *     first call to set up the slot without sending a SET_ADDRESS on
     *     the wire - some devices want a device descriptor read first.
     *     Linux does the BSR=1, then read 8-byte descriptor, then
     *     BSR=0 dance. Probably overkill for our HID-only use case.
     *
     *  6. Ring command doorbell (db 0, target 0). Wait for completion.
     *  7. On success, xhci.dcbaa[slot_id] now points at a device context
     *     the HC has updated with the assigned USB address (bits 7:0 of
     *     Slot Context's "device address" field).
     *
     * The thing that bites everyone the first time: forgetting to set the
     * "Context Entries" field in the Slot Context. The HC reads that field
     * to know how much of the input context is valid. */
    (void)slot; (void)port; (void)speed;
    return -1;
}


int xhci_configure_endpoint(xhci_slot_t* slot, usb_endpoint_descriptor_t* ep_desc) {
    /* TODO(you): T3 - Configure Endpoint.
     *
     * Computes DCI from ep_desc->bEndpointAddress, fills an EP Context in
     * the slot's Input Context, sets the corresponding A-flag, increments
     * the Slot Context's "Context Entries" to cover the new EP, then
     * issues a Configure Endpoint Command.
     *
     * For an interrupt-IN endpoint (HID):
     *   - DCI = 2*ep_num + 1 (IN endpoints are odd)
     *   - EP Type = XHCI_EPTYPE_INTERRUPT_IN
     *   - Max Packet Size = ep_desc->wMaxPacketSize
     *   - Interval: convert bInterval to xHCI's exponent form. Full-speed
     *     devices: Interval = log2(bInterval) + 3. High-speed: bInterval
     *     is already 2^(N-1) ms, so Interval = bInterval - 1. SuperSpeed
     *     mirrors high-speed.
     *   - Error Count = 3
     *   - Average TRB Length = ep_desc->wMaxPacketSize
     *   - TR Dequeue Pointer = phys of the EP's transfer ring (allocate
     *     a fresh ~32-entry ring here) with DCS=1.
     *
     * Same TRB plumbing as Address Device, different TRB type.
     */
    (void)slot; (void)ep_desc;
    return -1;
}


int xhci_control_transfer(xhci_slot_t* slot, usb_setup_packet_t* setup, void* data) {
    /* TODO(you): T4 - emit Setup TRB, optional Data TRB, Status TRB.
     *
     * Sequence (spec section 4.11.2.2):
     *
     *   1. Setup Stage TRB:
     *        parameter_lo = first 4 bytes of setup packet (bmRequestType,
     *                       bRequest, wValue low/high)
     *        parameter_hi = last  4 bytes of setup packet (wIndex,
     *                       wLength)
     *        status       = 8 (Transfer Length=8) << 0 | 0 (Interrupter)
     *        control      = TRB_TYPE(SETUP_STAGE) | TRB_CTRL_IDT
     *                       | TRT (Transfer Type) << 16
     *           TRT: 0=NoData, 2=OUT-Data, 3=IN-Data. Pick based on
     *           setup->bmRequestType direction bit.
     *
     *   2. If setup->wLength != 0, emit a Data Stage TRB:
     *        parameter    = phys(data)
     *        status       = wLength
     *        control      = TRB_TYPE(DATA_STAGE) | DIR bit
     *           DIR: 1=IN, 0=OUT (bit 16)
     *
     *   3. Status Stage TRB:
     *        control      = TRB_TYPE(STATUS_STAGE) | DIR bit
     *                       | TRB_CTRL_IOC
     *           The DIR for status is the OPPOSITE of the data stage
     *           (or 1=IN if no data stage).
     *
     *   4. Ring slot doorbell with target=1 (EP0 DCI).
     *   5. Block until completion event (cc=SUCCESS), return errno.
     */
    (void)slot; (void)setup; (void)data;
    return -1;
}


int xhci_queue_interrupt_in(xhci_slot_t* slot, u8 dci, void* buf, u32 len) {
    /* Queue a Normal TRB on the endpoint's transfer ring and ring the
     * doorbell. Completion comes via a Transfer Event handled in
     * xhci_handle_event. */
    xhci_trb_t trb = {0};
    u64 phys = xhci_va_to_dma(buf);
    trb.parameter_lo = (u32)(phys & 0xFFFFFFFFu);
    trb.parameter_hi = (u32)(phys >> 32);
    trb.status       = len & 0x1FFFF;
    trb.control      = TRB_SET_TYPE(TRB_TYPE_NORMAL) | TRB_CTRL_IOC | TRB_CTRL_ISP;

    xhci_ring_enqueue_trb(&slot->ep_rings[dci], &trb);
    ring_doorbell(slot->slot_id, dci);
    return 0;
}


/* ----------------------------------------------------------------------- *
 * Diagnostic dump.
 * ----------------------------------------------------------------------- */
void xhci_dump_state(void) {
    INFO("xhci: USBSTS=0x%x USBCMD=0x%x\n",
         op_rd32(0x04), op_rd32(0x00));
    for (u32 i = 1; i <= xhci.num_ports; i++) {
        volatile u32* portsc = XHCI_PORT_REG_BASE((u64)xhci.op, i);
        INFO("xhci: PORTSC[%d]=0x%x\n", i, *portsc);
    }
}


/* ----------------------------------------------------------------------- *
 * Top-level init.
 * ----------------------------------------------------------------------- */

bool xhci_init(u64 mmio_virt, u32 mmio_size) {
    xhci.mmio_base = mmio_virt;
    xhci.cap = (xhci_cap_regs_t*)mmio_virt;

    u8  caplen = (u8)(xhci.cap->caplength_hciversion & 0xFF);
    u16 ver    = (u16)(xhci.cap->caplength_hciversion >> 16);
    u32 hcs1   = xhci.cap->hcsparams1;
    u32 hccp1  = xhci.cap->hccparams1;
    u32 dboff  = xhci.cap->dboff;
    u32 rtsoff = xhci.cap->rtsoff;

    INFO("xhci: HCIVERSION 0x%x, CAPLENGTH 0x%x, HCSPARAMS1 0x%x\n",
         ver, caplen, hcs1);

    /* Sanity-check the cap regs before we trust them. If the VL805 isn't
     * responding to MMIO TLPs, the BCM2711 RC synthesises 0xDEADxxxx for
     * every read after its completion timeout (~50-100 ms per access).
     * Continuing past this point computes xhci.op = mmio_virt + caplen with
     * a garbage caplen, and the first MMIO load on Device memory will
     * alignment-fault (ESR.DFSC=0x21). Bail loudly instead. */
    if (ver == 0xDEAD || ver == 0xFFFF || ver == 0x0000
        || caplen < 0x20 || caplen > 0x80 || (caplen & 0x3)
        || hcs1 == 0xFFFFFFFFu || hcs1 == 0xDEADDEADu) {
        ERROR("xhci: cap regs look like garbage (ver=0x%x caplen=0x%x "
              "hcs1=0x%x). VL805 not responding to MMIO - PCIe link is "
              "up but TLP completions are timing out. Check that the "
              "VL805 firmware reload succeeded and that BAR0 / outbound "
              "window addresses match.\n", ver, caplen, hcs1);
        return FALSE;
    }

    xhci.op        = (xhci_op_regs_t*)(mmio_virt + caplen);
    xhci.ir0       = (xhci_intr_regs_t*)(mmio_virt + (rtsoff & ~0x1Fu) + 0x20);
    xhci.doorbells = (volatile u32*)(mmio_virt + (dboff & ~0x3u));
    xhci.num_slots = XHCI_HCS1_MAXSLOTS(hcs1);
    xhci.num_ports = XHCI_HCS1_MAXPORTS(hcs1);
    xhci.ctx_64    = (hccp1 & XHCI_HCCP1_CSZ) != 0;

    INFO("xhci: %d slots, %d ports, ctx_64=%d\n",
         xhci.num_slots, xhci.num_ports, xhci.ctx_64);

    if (xhci.ctx_64) {
        WARNING("xhci: controller reports 64-byte contexts; driver "
                "assumes 32-byte. Add 64-byte support before trusting.\n");
    }

    if (!hc_reset())            return FALSE;

    /* Detect required page size. PAGESIZE has the supported sizes as bits,
     * the smallest set bit being the natural minimum. VL805 reports 1 (=4K). */
    u32 ps = op_rd32(0x08);
    xhci.page_size_bytes = (ps & ~(ps - 1)) << 12;
    DEBUG("xhci: PAGESIZE register = 0x%x (=> %d byte natural page)\n",
          ps, xhci.page_size_bytes);

    /* Enable the slot bank we plan to use. */
    op_wr32(0x38, xhci.num_slots);    /* CONFIG.MaxSlotsEn = MaxSlots */

    if (!dcbaa_init())          return FALSE;
    if (!cmd_ring_init())       return FALSE;
    if (!event_ring_init())     return FALSE;

    /* Enable our SPI in the GIC distributor. The existing kernel only
     * arms PPIs (timer = INTID 30); SPIs need explicit ISENABLER + a
     * priority below the CPU PMR (0xFF). If GICD is not yet globally
     * enabled (GICD_CTRLR), we set it here too. */
    {
        u32 idx = PCIE_INTID_LEGACY / 32;
        u32 bit = PCIE_INTID_LEGACY % 32;
        REGS_GICD->gicd_ipriorityr[PCIE_INTID_LEGACY / 4] |=
            (0x80u << ((PCIE_INTID_LEGACY % 4) * 8));
        REGS_GICD->gicd_isenabler[idx] = (1u << bit);
        if (!(REGS_GICD->gicd_ctlr & 1u)) REGS_GICD->gicd_ctlr |= 1u;
    }

    /* Now flip RS and INTE and let the controller run. */
    op_wr32(0x00, XHCI_USBCMD_RS | XHCI_USBCMD_INTE);

    /* Wait for HCH to clear. */
    for (int i = 0; i < 100; i++) {
        if (!(op_rd32(0x04) & XHCI_USBSTS_HCH)) {
            INFO("xhci: controller running\n");
            xhci_dump_state();
            return TRUE;
        }
        timer_sleep(1);
    }
    ERROR("xhci: HC failed to come out of halt after RS=1\n");
    return FALSE;
}
