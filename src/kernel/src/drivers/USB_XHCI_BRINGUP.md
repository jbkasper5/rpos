# USB / xHCI bring-up on Pi 4B (BCM2711 + VL805)

> **Note:** This document was written with substantial help from an AI assistant during the bring-up debugging cycle. The implementation, the bugs we hit, and the resolutions are real — the structure and prose owe a lot to that collaboration. Treat it as a learning aid rather than a polished reference; cross-check anything load-bearing against the Linux kernel's `drivers/pci/controller/pcie-brcmstb.c` and `drivers/usb/host/xhci.c`, and against u-boot's `drivers/pci/pcie_brcmstb.c` in this repo at `resources/u-boot/`.

This doc covers what you actually need to do, in what order, and **why each step matters** to get from "the Pi just booted" to "the kernel can talk to a USB keyboard." It deliberately spends more time on the *why* than on register values you can grep from the source.

---

## 1. The physical stack

Plugging a USB keyboard into a USB-A port on a Raspberry Pi 4B traverses, from the device upward:

```
USB keyboard
   │  USB wire protocol (low-speed, full-speed, high-speed, SuperSpeed)
   ▼
VL805 xHCI host controller    ← VIA Labs chip on the Pi PCB, separate SoC
   │  PCIe (link layer + memory/config TLPs)
   ▼
BCM2711 PCIe Root Complex     ← inside the SoC
   │  AXI / SoC memory fabric
   ▼
ARM Cortex-A72 cores          ← runs your kernel
```

Three completely separate subsystems collaborating:

- **VL805**: the actual xHCI controller. Speaks USB downstream and PCIe upstream. Has its own firmware, loaded from either its own SPI EEPROM (Pi 4B rev 1.4+) or from the SoC's VideoCore over an I²C side-channel (older revs).
- **BCM2711 PCIe Root Complex**: a Broadcom block in the SoC that bridges between the ARM/AXI side and the VL805's PCIe link. This is the part that needs the most non-standard configuration.
- **xHCI protocol**: the standardized "how the OS talks to a USB host controller" interface, identical regardless of which physical xHCI controller you have.

> The USB-C port on the Pi 4B is *not* on the VL805. It goes through the SoC's internal DWC2 controller. This driver is exclusively for the four USB-A ports.

---

## 2. The overall bring-up order

```
1.  Power on the PCIe/USB rail        (mailbox to GPU)
2.  Map RC + outbound window MMIO     (page tables)
3.  Reset the PCIe Root Complex       (INIT + PERST dance)
4.  Configure RC internal state       (MISC_CTRL, SerDes, inbound window)
5.  Release PERST, wait for link up
6.  Configure bridge forwarding       (class code, MEM_BASE/LIMIT, CMD)
7.  Set bus numbers on the bridge
8.  Enumerate VL805 over PCIe         (config space: VID/DID, BAR, CMD)
9.  Tell the GPU to reload VL805 firmware  (mailbox: NOTIFY_XHCI_RESET)
10. xHCI controller bring-up          (cap regs, reset, structures, RS=1)
11. Port scan + device enumeration    (the actual USB protocol)
```

Steps 1–9 are PCIe and Pi-specific. Step 10 is xHCI-spec-standard. Step 11 starts looking like normal USB.

---

## 3. PCIe bring-up (`drivers/pcie.c`)

### 3.1 Power-on the rail

The PCIe controller, the VL805, and the USB-A power pins all share one BCM2711 power domain ("USB"). It's off at SoC reset. The GPU firmware exposes it via mailbox tag `RPI_FIRMWARE_SET_DOMAIN_STATE` (0x38030) for domain ID 6.

- Don't use `SET_POWER_STATE` (0x28001) — that tag's IDs are *device* IDs (SD=0, UART0=1, ...), and ID 6 there is I2C2, not USB. This bit you in a different decade if you misread the docs.
- There is no PCIe clock to enable via `SET_CLOCK_STATE`; PCIe clocking is internal.

After this call the rail is alive but the RC is still in reset.

### 3.2 Map MMIO

Two regions need to be in the kernel's virtual address space before we touch anything:

| Region | Phys | Size | Why |
|---|---|---|---|
| **PCIe RC registers** | `0xFD500000` | 64 KiB | The RC's own control/status block. We poke this to configure the bridge. |
| **PCIe outbound window** | `0x600000000` (24 GiB) | 1 MiB (we only need 64 KiB for VL805, 1 MiB is conservative) | The CPU-side phys address range that gets translated into PCIe memory-space TLPs targeting the VL805's BAR. |

Both are mapped as `MAP_DEVICE | MAP_NOCACHE`, which on AArch64 means **Device-nGnRnE** memory in MAIR. This is mandatory for MMIO. The CPU treats Normal-cached memory completely differently from Device memory:

- **Normal memory**: speculative reads, write coalescing, reordering across same-region accesses, the works.
- **Device memory**: strict ordering, no speculation, no coalescing, no merging, no early write-ack.

The strictness is why we crash with an alignment fault if we ever do a misaligned word access to PCIe MMIO — which is exactly how the *very first* crash in our debugging journey manifested.

### 3.3 The reset dance

`PCIE_RGR1_SW_INIT_1` (offset `0x9210` in the RC block) has two reset bits, both asserted at POR:

- **bit 0 = PERST**: downstream link reset visible to the device. While asserted, the VL805 sees its PERSTN pin held low and stays in reset.
- **bit 1 = INIT**: the RC's own internal reset.

Sequence:

```
1. Assert both (PERST=1, INIT=1)         ← at this point everything is in reset
2. Wait briefly                          ← guard against the reset signal looking like a glitch
3. Deassert INIT only (PERST stays=1)    ← now the RC starts running, link still held down
4. ... configure RC internal state ...
5. Deassert PERST                        ← VL805 comes out of reset, link trains
6. Wait 100 ms per PCIe CEM spec §2.2
7. Poll for link up
```

The key here is **step 6 is non-negotiable**. The PCIe Card Electromechanical spec requires 100 ms between PERST# deassertion and the first config TLP. Skip it and the device drops your TLPs silently.

### 3.4 SerDes IDDQ — the most important single bit

`PCIE_MISC_HARD_PCIE_HARD_DEBUG` register, **bit 27** (`SERDES_IDDQ_MASK = 0x08000000`):

> "IDDQ" = "Quiescent Current," industry slang for "low-power-mode where most of the analog block is off."

At power-on this bit is **set**, which parks the SerDes PHY in a low-power state. The strange thing is: **link training still completes superficially** — `PHYLINKUP` and `DL_ACTIVE` both go high — and **config-space TLPs still work**, because they take a separate internal path through the RC. But **memory-space TLPs get dropped** without completion, and the RC's TLP completion timeout (~50-100 ms) eventually synthesizes `0xDEADxxxx` as the read value.

This is what causes the canonical "config works, memory doesn't" symptom on a fresh BCM2711 boot. **Clear this bit early**, right after INIT deassert, before configuring anything else.

After clearing, wait a bit (u-boot uses 100 µs; we use 1 ms) for the SerDes to stabilize.

### 3.5 MISC_CTRL

The Broadcom-proprietary `PCIE_MISC_MISC_CTRL` register holds several knobs:

- `SCB_ACCESS_EN`: enable the **inbound** memory access path. Without this, the VL805 can't DMA into DRAM.
- `CFG_READ_UR_MODE`: when a config read targets a nonexistent device, return UR (Unsupported Request) instead of timing out. Speeds up enumeration of empty slots — though we know there's exactly one device on bus 1.
- `MAX_BURST_SIZE_128`: limit AXI bursts to 128 bytes (the VL805's sweet spot).
- `SCB0_SIZE`: log₂(inbound window size) − 15. For 1 GiB that's 15.

### 3.6 Inbound vs outbound — two completely separate windows

This trips people up. The BCM2711 PCIe RC has two independent address-translation paths:

- **Inbound** (`RC_BAR2_CONFIG_LO/HI`): "device → memory." The VL805 sends a DMA write to PCIe bus address X; the RC translates X into a CPU physical address. We configure it as a 1:1 map of DRAM[0..1 GiB] at bus address 0..1 GiB. This is how the VL805 reads its TRBs, contexts, and event ring entries out of DRAM.
- **Outbound** (`PCIE_MISC_CPU_2_PCIE_MEM_WIN0_*`): "CPU → device." The CPU writes to physical address X (which is mapped to a virtual address by our page tables); the RC sees the write on its AXI port and translates X into a PCIe memory TLP at bus address Y. We configure it to map CPU phys `0x6_0000_0000..0x6_000F_FFFF` to PCIe bus `0xF8000000..0xF80FFFFF`. This is how the CPU reads/writes the xHCI's MMIO registers.

We program both. They are independent; getting one right doesn't help with the other.

### 3.7 Outbound window encoding (subtle!)

`PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT` is a 32-bit register that packs two 12-bit fields, **at non-obvious bit positions**:

| Bits  | Field | What |
|---|---|---|
| `[15:4]`  | `BASE`  | CPU phys base[31:20] (in MiB units) |
| `[31:20]` | `LIMIT` | CPU phys limit[31:20] (in MiB units) |
| `[3:0]`, `[19:16]` | reserved | leave zero |

Plus two separate registers for the high bits:
- `BASE_HI[7:0]` = `phys_addr_mb >> 12` (the part of the MiB count above bit 12)
- `LIMIT_HI[7:0]` = same for limit

For our CPU base of `0x6_0000_0000`: `phys_addr_mb = 0x6000`, the low 12 bits go to `BASE` (= `0x000`), and the rest goes to `BASE_HI` (= `0x6`). It's easy to write `(cpu_base >> 20) << 4` and assume that's the BASE field; for our specific addresses it happens to give the same result (since the BASE field zeroes out), but **for any other CPU base it's wrong**. The first version of this driver got this wrong and survived purely by coincidence of the address choice.

### 3.8 Bus numbers and bridge forwarding

The BCM2711 PCIe RC presents itself as a **PCI-to-PCI bridge** with a **Type 1 config header** sitting at bus 0, device 0, function 0. The VL805 is at bus 1, device 0, function 0 — behind the bridge.

For the bridge to forward TLPs at all, three things must be programmed:

1. **Bus Number register** (config offset `0x18`): `prim=0, sec=1, sub=1`. Without this, config TLPs targeting bus 1 return Unsupported Request, surfacing to us as `VID:DID = 0xFFFF:0xFFFF`.
2. **Memory Base/Limit** (config offset `0x20`): the address range the bridge will forward memory TLPs to. For us, `[0xF8000000, 0xF80FFFFF]`. **Without this, memory TLPs are dropped silently even with the proprietary outbound window programmed.** This was the last big bug we hit; took half a day to track down because every diagnostic-readback looked correct.
3. **Bridge's Command register** (config offset `0x04`): `MEM | MASTER` bits. Without `MEM`, the bridge ignores memory TLPs from the primary bus. Without `MASTER`, it can't forward upstream DMA from the VL805 back to CPU memory.

Linux and u-boot don't program (2) and (3) explicitly because their generic PCI subsystem does it during enumeration. We do enumeration manually, so we have to do it ourselves. **This is the single most easily-missed step for a baremetal xHCI driver.**

### 3.9 Bridge class code

`PCIE_RC_CFG_PRIV1_ID_VAL3` (RC internal register, *not* standard PCI config space) needs its low 24 bits set to `0x060400` — class = Bridge, subclass = PCI-to-PCI bridge, prog-if = Normal Decode. The reset default is "Endpoint" class. **Generic PCI code looking at this header to decide TLP-forwarding behavior will refuse to enumerate downstream buses if the class says Endpoint**, but the BCM2711 RC's own forwarding *also* looks at this field — at least empirically, getting it wrong was correlated with TLP-completion timeouts during our debugging.

### 3.10 Endian mode and ASPM

Two RC config-space writes done after link is up:

- `PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1.ENDIAN_MODE_BAR2 = LITTLE_ENDIAN`: forces little-endian byte ordering on the inbound BAR2 path. The reset value of this field is unspecified.
- `PCIE_RC_CFG_PRIV1_LINK_CAPABILITY.ASPM_SUPPORT = 0`: unadvertises ASPM support. ASPM (Active State Power Management) lets the link drop into L0s/L1 low-power states between bursts, but the VL805 has buggy CLKREQ# handling and the exit-from-L1 sequence sometimes fails. Bootloaders unconditionally disable it.

### 3.11 Order matters — pre- vs post-link-up

This is non-obvious and bit us hard during debugging. The PCIe RC's CFG-space (`PCIE_RC_CFG_*`) gets re-initialized to defaults when the link trains. So any writes to:

- Bridge class code (`PCIE_RC_CFG_PRIV1_ID_VAL3`)
- Endian mode (`PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1`)
- ASPM (`PCIE_RC_CFG_PRIV1_LINK_CAPABILITY`)

that happen *before* PERST is released get **clobbered by link training**. You won't see anything happen — the writes complete, the readbacks just before PERST release show the values you wrote — but as soon as the link trains, the fields reset.

The MISC block (`PCIE_MISC_*`, including the outbound window registers) is not affected by link training, but for consistency with u-boot and to remove any cognitive load about which registers reset and which don't, we set up everything we can post-link.

### 3.12 The VL805's own firmware

After the link is up and the VL805 is enumerable in config space, its xHCI logic still doesn't respond — because its firmware hasn't been loaded yet. The VL805 has two firmware-load mechanisms:

- **Pi 4B rev 1.4+**: the VL805 has its own SPI EEPROM, and it auto-loads from there when PERST# deasserts. The GPU firmware does nothing extra.
- **Older revs**: the VL805 has no local EEPROM. The SoC's GPU is wired to it over an I²C side-channel and uploads firmware after boot.

The kernel side dispatches a `RPI_FIRMWARE_NOTIFY_XHCI_RESET` mailbox tag (`0x00030058`) to tell the GPU "load VL805 firmware." On a rev 1.4+ board this is essentially a no-op (the GPU returns success without doing anything because the VL805 already loaded itself). On an older board this triggers the I²C upload. Either way, the call must succeed.

Then **wait ~200 ms** for the firmware to actually be loaded and the xHCI logic to come online. u-boot uses `udelay(200)`; we use 200 ms because we have it. Whichever — the point is that the mailbox tag returning success means "the GPU accepted the request," not "the firmware is now running."

---

## 4. Why we crashed at `caplen = 0xAD`

Connecting the journey: when *any* of the above is wrong (SerDes IDDQ not cleared, bridge MEM_BASE/LIMIT not set, bridge CMD.Memory not enabled, firmware not loaded), the symptom on the kernel side is identical:

1. CPU reads `xhci.cap->caplength_hciversion` at virt `0xFFFF800600000000`.
2. MMU translates to phys `0x600000000`.
3. Outbound window translates to PCIe bus `0xF8000000`.
4. PCIe TLP travels down the link to the VL805.
5. **No completion comes back.**
6. After ~50-100 ms, the BCM2711 RC's TLP-completion timeout fires.
7. The RC synthesizes `0xDEADxxxx` as the "data" the CPU read would have gotten.

The CPU reads `0xDEADxxxx` as if it were a real value. `caplength_hciversion & 0xFF = 0xAD`. Then the driver computes `xhci.op = mmio_virt + 0xAD` — an unaligned address. The next register read is a 4-byte load from `mmio_virt + 0xAD`, which **misaligns on Device memory** → AArch64 throws an alignment fault (ESR.DFSC = 0x21).

The "0xDEAD" signature is the loudest possible signal that PCIe is mis-configured. The alignment fault that follows is a downstream consequence. We added a sanity check (`xhci_init` guard) that compares HCIVERSION and CAPLENGTH against plausible ranges and bails with a clear error before computing `xhci.op`, so you get a meaningful message instead of a synchronous exception 50 ms later.

---

## 5. xHCI controller initialization (`drivers/xhci.c`)

Once the VL805's MMIO actually responds, the rest is the standardized xHCI spec ("eXtensible Host Controller Interface for USB"), which is the same regardless of which chip implements it.

### 5.1 Read capability registers

Located at offset 0 of the MMIO BAR. Fixed layout:

| Offset | Reg | Decoded |
|---|---|---|
| `0x00` | `CAPLENGTH` (low byte) + `HCIVERSION` (high 16) | CAPLENGTH = where the operational registers start. For VL805: `0x20`. HCIVERSION = `0x0100` (xHCI spec 1.0). |
| `0x04` | `HCSPARAMS1` | Max slots, max interrupters, max ports. For VL805: `0x05000420` → 32 slots, 4 interrupters, 5 ports. |
| `0x08` | `HCSPARAMS2` | Scratchpad buffer count, etc. |
| `0x0C` | `HCSPARAMS3` | Power/exit latency. |
| `0x10` | `HCCPARAMS1` | Capability flags. Bit 2 = CSZ (Context Size: 0 = 32-byte, 1 = 64-byte). For VL805: 0 = 32-byte. |
| `0x14` | `DBOFF` | Doorbell array offset from MMIO base. |
| `0x18` | `RTSOFF` | Runtime registers offset (interrupter contexts). |

The driver reads these into its state struct, then computes:
- `op_base = mmio + caplength`
- `rt_base = mmio + rtsoff`
- `db_base = mmio + dboff`

If `caplength` is bogus (e.g., `0xAD` from a 0xDEAD read), every subsequent register access is at a wrong-and-misaligned address. This is why we sanity-check it.

### 5.2 Halt + reset the controller

Operational register `USBCMD` (offset 0x00 of op-regs):

```c
USBCMD.RS = 0           /* halt */
wait for USBSTS.HCH=1   /* up to 100 ms */
USBCMD.HCRST = 1        /* reset */
wait for HCRST to self-clear AND USBSTS.CNR=0  /* up to 1000 ms */
```

`HCH` = Host Controller Halted. `CNR` = Controller Not Ready. After a clean reset, both go low.

### 5.3 Detect required page size

Operational register `PAGESIZE` (offset 0x08). Reports as a bitmask of supported sizes; the smallest set bit, shifted left by 12, gives the natural page size. For VL805: `0x1` → 4 KiB.

This is the alignment requirement for every DMA structure we allocate (DCBAA, command ring, event ring, ERST, contexts).

### 5.4 Allocate the DCBAA

The **Device Context Base Address Array** is a contiguous array of 8-byte pointers, one per slot, indexed by slot ID. Slot 0 is special: it's not a real slot, but its DCBAA entry points to a "scratchpad buffer array" if the HC needs scratchpad pages (`HCSPARAMS2.MaxScratchpadBufs`).

```
DCBAA virt address: kmalloc'd, page-aligned
DCBAA phys address: written to op-reg DCBAAP (offset 0x30)
```

For VL805 with 32 slots: `33 * 8 = 264 bytes`, rounded up to one 4 KiB page.

### 5.5 Allocate the command ring

A ring of 256 TRBs (each 16 bytes = 4 KiB total). The last TRB is a **Link TRB** pointing back to the start, with `TOGGLE_CYCLE` set.

```
Command ring virt:  kmalloc'd
Command ring phys:  written to op-reg CRCR (offset 0x18), OR'd with RCS=1
```

The driver writes commands into this ring; the HC reads them.

### 5.6 Allocate the event ring + ERST

Symmetric to the command ring but in the *other direction*: the HC writes events into the event ring; the driver reads them.

The event ring is described to the HC via a separate structure, the **Event Ring Segment Table** (ERST). Even for a single-segment event ring, you need a 1-entry ERST.

```
ERST virt: 16 bytes, contains { ring_segment_base, ring_segment_size, reserved }
Event ring virt: 256 TRBs
```

Three writes to interrupter 0's registers in the runtime block, **in this order**:

```
IR0.ERSTSZ = 1                          /* number of ERST segments */
IR0.ERDP   = phys(event ring start)     /* dequeue pointer */
IR0.ERSTBA = phys(ERST)                 /* base addr — write last! */
```

Writing `ERSTBA` is the trigger that tells the HC "now go look at ERSTSZ and ERDP." Writing it last is required by spec.

### 5.7 Enable interrupter 0

```
IR0.IMAN.IE = 1     /* Interrupt Enable */
IR0.IMOD = default  /* Interrupt Moderation; default of 4000 ≈ 1 ms is fine */
```

`IMAN.IP` (Interrupt Pending) is write-1-to-clear and is acknowledged in the IRQ handler.

### 5.8 Enable the GIC SPI

For Pi 4B, the PCIe legacy IRQ is **SPI 147 = INTID 179** (SPI offset 32). The kernel needs to:

```
GICD_IPRIORITYR: set priority < PMR (i.e., 0x80)
GICD_ISENABLER: enable bit for INTID 179
GICD_CTRLR: bit 0 = 1 (enable distributor — usually already on)
```

The PCIe RC also has its own INTR2 block (`PCIE_INTR2_*` registers) that gates which sources are forwarded to the GIC line. We mask MSI during bring-up (`PCIE_MSI_INTR2_MASK_SET = 0xFFFFFFFF`) and unmask legacy INTA-INTD when we're ready to receive them.

### 5.9 Flip the run bit

```
USBCMD.RS = 1       /* Run/Stop */
USBCMD.INTE = 1     /* Interrupter Enable */
wait for USBSTS.HCH = 0
```

The HC starts running. From this moment, port status changes generate events on the event ring.

---

## 6. Producer-consumer rings and the cycle bit

xHCI has three kinds of rings, all with the same underlying shape:

| Ring | Producer | Consumer | Purpose |
|---|---|---|---|
| **Command ring** | driver | HC | "do X" commands like Enable Slot, Address Device |
| **Transfer rings** (one per endpoint) | driver | HC | data transfers (control, bulk, interrupt) |
| **Event ring** | HC | driver | command completions, transfer completions, port changes |

All three are circular arrays of 16-byte TRBs (Transfer Request Blocks), but **the cycle bit replaces head/tail pointers** as the empty/full indicator. Here's the mechanism:

- Each ring has a **producer cycle state** (PCS) of 0 or 1, owned by the producer.
- Each TRB has a **cycle bit** (bit 0 of the control dword).
- The producer writes the TRB *with* PCS as the cycle bit.
- The consumer maintains its own **consumer cycle state** (CCS), initially 1 for the command/transfer rings (matching the HC's initial expectation) and 1 for the event ring (matching what the HC produces).
- The consumer reads a TRB; if `TRB.cycle == CCS`, it's a valid TRB to process. Otherwise, the ring is empty from the consumer's perspective.
- The consumer processes the TRB, then advances its dequeue index.

When the producer or consumer reaches the end of the ring (the **Link TRB** at the last slot), it:
1. Follows the Link TRB's destination pointer back to the start of the ring.
2. If the Link TRB has `TOGGLE_CYCLE = 1`, the producer/consumer flips its own cycle state.

This is the core abstraction. It's the same shape as the pipe ring buffer with head/tail, except instead of explicit indices, the "is this slot full / empty" signal is encoded in the data itself. The advantage: the HC and the driver don't need to communicate dequeue/enqueue indices on every operation — each just looks at the cycle bit of the next TRB.

### 6.1 Enqueuing a TRB (producer side)

```
1. Write parameter and status fields of trb[enqueue_idx] (12 bytes).
2. dsb sy   ← ensure those writes are visible to the HC.
3. Write control dword of trb[enqueue_idx] WITH cycle bit = PCS.
4. dsb sy   ← so the HC sees the complete TRB before being told via doorbell.
5. enqueue_idx++.
6. If enqueue_idx is now the Link TRB's slot:
     a. Update Link TRB's cycle bit to PCS.
     b. Flip PCS.
     c. enqueue_idx = 0.
7. Ring the doorbell.
```

The "write cycle bit last" rule is critical. If the HC happens to be polling this slot when you're halfway through writing the TRB, it could process garbage. The spec calls this "the host shall write the Cycle bit last."

### 6.2 Dequeuing an event (consumer side)

```
1. ev = event_ring[dequeue_idx]
2. If ev.cycle != CCS: ring is empty; bail.
3. Process the event.
4. dequeue_idx++. If dequeue_idx == ring_size: dequeue_idx = 0; CCS ^= 1.
5. Update IR0.ERDP with phys(event_ring[dequeue_idx]) | EHB (Event Handler Busy, write-1-to-clear).
```

Writing ERDP with EHB set tells the HC "I've processed events up to here, you may raise another interrupt now."

---

## 7. Device enumeration (what happens when you plug in a keyboard)

This is the protocol-level flow, after the controller is running:

```
1. Device is connected to a port.
2. HC detects connect → writes a Port Status Change Event onto the event ring.
3. Driver's IRQ handler processes the event, calls usb_port_change(port).
4. usb_port_change resets the port (PORTSC.PR = 1, wait for PRC).
5. After reset, PORTSC.CCS=1, PORTSC.Speed indicates speed.
6. Driver issues Enable Slot Command → HC responds with a Slot ID.
7. Driver allocates Device Context and Input Context.
   - Input Context: { Input Control, Slot Context, EP0 Context }
   - Sets RouteString=0, Speed, Context Entries=1, Root Hub Port.
8. Driver issues Address Device Command → HC assigns a USB address.
9. Driver issues GET_DESCRIPTOR(DEVICE, 8) over EP0 → learns the real EP0 MaxPacketSize.
10. (Optional: Evaluate Context to update EP0 MPS for low/full speed devices.)
11. Driver issues GET_DESCRIPTOR(DEVICE, full) → vendor/product IDs, class info.
12. Driver issues GET_DESCRIPTOR(CONFIGURATION) → parses interfaces and endpoints,
    looking for an HID interface with boot subclass.
13. Driver issues SET_CONFIGURATION(value) → activates the config.
14. Driver issues SET_PROTOCOL(BOOT) + SET_IDLE(0) → forces simple report format.
15. Driver issues Configure Endpoint Command → adds the IN endpoint to the slot's
    Device Context with its own transfer ring.
16. Driver queues a Normal TRB on the IN endpoint's transfer ring → HC starts
    polling the device at the requested interval.
17. On each completed transfer (a key pressed), the HC writes a Transfer Event
    onto the event ring, which dispatches to the HID layer.
```

The five `TODO(you):` sites in `xhci.c` correspond to:

- **T1**: `xhci_ring_enqueue_trb` — the producer logic from §6.1.
- **T2**: `xhci_address_device` — steps 7–8.
- **T3**: `xhci_configure_endpoint` — step 15.
- **T4**: `xhci_control_transfer` — the SETUP/DATA/STATUS TRB sequence for steps 9–14.
- **T5**: completion-code → errno mapping — used by all of the above when they wait for an event.

---

## 8. Common failure modes and how to diagnose

| Symptom | Most likely cause |
|---|---|
| `HCIVERSION = 0xDEAD`, ~50 ms read latency | PCIe memory TLP completion timeout. Check (in order): SerDes IDDQ cleared? Bridge `CMD.Memory` set? Bridge `MEM_BASE/LIMIT` covers BAR? Outbound window covers BAR PCIe addr? Firmware loaded? |
| Alignment fault at `mmio + 0xAD` | Downstream of 0xDEAD: bogus `CAPLENGTH` extracted from 0xDEAD reads. Fix the PCIe issue above; the `xhci_init` guard prevents this fault from happening as a side effect. |
| Link doesn't come up | PERST timing? 100 ms post-deassert delay present? Check `PCIE_MISC_PCIE_STATUS` — both `PHYLINKUP` and `DL_ACTIVE` must be set. |
| `VID:DID = 0xFFFF:0xFFFF` | Bus numbers not programmed, or device behind a different bus than expected, or link not up. |
| BAR latches as `0xFFFFFFFx` after `pci_config_write32` | Writing while the device isn't ready, or BAR is being sized — re-read of all-ones is the device telling you "this is my size mask." Wait a bit and re-write. |
| `pcie: NOTIFY_XHCI_RESET ... not processed` | Pi firmware too old to know the tag. Update bootloader EEPROM (`rpi-eeprom-update -a`) and/or `start4.elf` from current `raspberrypi/firmware` master. |
| HC reset never completes | RC isn't actually talking to the VL805. Same checklist as 0xDEAD. |
| `CSZ=1` (64-byte contexts) | Add 64-byte context layout support. Or — much more likely if you're seeing this *together with* 0xDEAD reads — it's a false positive from garbage HCCPARAMS1, and resolves itself once PCIe works. |
| Port stays at CCS=0 | Device not connected, port disabled, or driver hasn't called PORTSC.PP=1 yet (Port Power). Most VL805 ports come up with PP=1 by default but worth checking. |

---

## 9. References

- **xHCI spec**: Intel "eXtensible Host Controller Interface for Universal Serial Bus," current rev 1.2. Sections 4.6 (commands), 4.9 (rings), 4.11 (transfers), 6 (register definitions).
- **PCIe CEM spec**: PCI Express Card Electromechanical Specification, §2.2 for PERST# timing.
- **PCI 3.0 spec**: §6.1 for Type 1 bridge headers (MEM_BASE/LIMIT, CMD register).
- **Linux**: `drivers/pci/controller/pcie-brcmstb.c` (RC bring-up), `drivers/usb/host/xhci.c` (xHCI), `drivers/input/touchscreen/raspberrypi-ts.c` (touchscreen if relevant later).
- **u-boot** (in this repo): `resources/u-boot/drivers/pci/pcie_brcmstb.c`, `resources/u-boot/arch/arm/mach-bcm283x/msg.c::bcm2711_notify_vl805_reset`.
- **rpi-firmware mailbox**: tag `RPI_FIRMWARE_NOTIFY_XHCI_RESET = 0x00030058`, payload one `u32` device address `(bus << 20) | (dev << 15) | (fn << 12)`.

---

## 10. Lessons that generalize beyond xHCI

These came out of the debugging journey and apply to almost any "talk to a device over a bus" driver:

1. **"It looks like 0xDEAD or 0xFFFF" is the loudest possible signal.** Don't trust a single byte of it. Sanity-check magic values from MMIO reads before computing addresses from them. A 5-line guard turns "alignment fault deep in init" into "clear error message before the next access."

2. **Config space and memory space go through completely separate paths.** "Config space works" tells you the bridge's bus-number-based routing is right; it tells you *nothing* about memory-space forwarding. The two failure modes look identical from outside, which is what makes this class of bug nasty.

3. **Bus protocols have multiple layers of forwarding state.** On PCIe alone: the SoC's address decoder (CPU phys → PCIe block), the RC's outbound window (CPU phys → PCIe bus addr), the bridge's MEM_BASE/LIMIT (forward TLPs matching this range), the bridge's CMD register (enable forwarding at all), the device's BAR (claim this PCIe range), the device's CMD register (respond to MEM accesses). Any one being wrong gives the same surface symptom: "no completion."

4. **Defaults are not your friend.** Anything not explicitly programmed is whatever the SoC vendor felt like at fab time. For PCIe: SerDes is parked, ASPM is enabled, endian is undefined, class code is "Endpoint." These are all wrong for our use case. Program everything that matters.

5. **Mailbox calls return success ≠ the work got done.** GPU firmware acknowledges receipt of a tag long before it finishes executing the request. Always add a settling delay sized to whatever's actually happening (200 ms for VL805 firmware over I²C, 100 ms for PERST CEM compliance, etc.).

6. **Generic kernels do enumeration for you. Baremetal kernels don't.** Don't assume a register field is "default-OK" just because Linux doesn't program it explicitly — Linux's PCI subsystem programs it during enumeration. We had to discover this for `MEM_BASE/LIMIT` and `CMD` on the bridge, and there may be others we're still relying on default values for.

7. **Order matters more than you think.** Some RC registers reset on link training; some don't. Write the link-training-sensitive ones after link-up. When in doubt, follow u-boot's order to the letter.

8. **Cycle bit ≈ head/tail pointer in disguise.** Internalize the producer/consumer ring abstraction with the cycle bit once and you can read xHCI, NVMe, virtio, and most modern device interfaces without re-learning.
