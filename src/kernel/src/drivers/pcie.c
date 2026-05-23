/*
 * BCM2711 PCIe Root Complex bring-up + VL805 enumeration.
 *
 * What this file does, end-to-end:
 *
 *   1. Ask the GPU firmware (via the mailbox) to power on the PCIe rail.
 *   2. Map the RC's register window (0xFD500000) into kernel virtual space
 *      as MAP_DEVICE.
 *   3. Drive the RC out of reset, then drop PERSTN to release the downstream
 *      device, wait for link up.
 *   4. Configure the outbound CPU->PCIe memory window so accesses to virt
 *      addrs in the 0x600000000 region get translated to PCIe BAR space.
 *   5. Configure the inbound PCIe->DRAM window so the VL805 can DMA into
 *      the first 1 GiB of system RAM 1:1.
 *   6. Walk PCI config space on bus 1 looking for the VL805. Once found,
 *      program its BAR0 to a fixed PCIe address, set bus master + memory
 *      space in the command register, and report the result to the caller.
 *
 * Reference implementations cross-checked against:
 *   - Linux drivers/pci/controller/pcie-brcmstb.c
 *   - Circle (Pi bare-metal) circle/bcmpciehostbridge.cpp
 *
 * Caveats / known-good range:
 *   - Tested only against a Pi 4B Rev 1.4+ (where the VL805 firmware is
 *     pre-loaded by the GPU bootloader from the SoC EEPROM). On older
 *     revisions you'd additionally need to send mailbox tag 0x00030058
 *     (RPI_FIRMWARE_NOTIFY_XHCI_RESET) after enumeration.
 */

#include "macros.h"
#include "io/kprintf.h"
#include "utils/timer.h"
#include "utils/utils.h"
#include "memory/mmap.h"
#include "memory/paging.h"
#include "mailbox/mailbox.h"
#include "drivers/pcie.h"

/* ---- Tiny MMIO wrappers ------------------------------------------------- *
 * Centralised so we can drop in barriers as needed. xHCI on Pi 4B is
 * mostly cache-coherent, but a `dmb sy` between config writes is cheap
 * insurance against compiler/hw reordering. */

static inline u32 rd32(u64 va)        { return *(volatile u32*)va; }
static inline void wr32(u64 va, u32 v){ *(volatile u32*)va = v; asm volatile("dmb sy"); }

#define RC(off)  (PCIE_RC_BASE + (off))


/* ----------------------------------------------------------------------- *
 * Step 1: map the RC registers and the PCIe outbound memory window.
 * ----------------------------------------------------------------------- */

/*
 * Map the two MMIO regions we need. We deliberately use map() rather than
 * map_pages() so the kernel's dispatcher routes us through map_blocks for
 * 2 MiB-aligned regions - that's the same path PBASE/GIC use successfully
 * during finish_virtual_mapping(), and it avoids exercising the L3 path
 * in map_pages() which empirically falls over at runtime for brand-new
 * subtrees.
 *
 * Sizing in block_order (= log2 of pages):
 *   - PCIE_RC is at 0xFD500000, which is 1 MiB into a 2 MiB block (not
 *     2 MiB aligned), so map() will fall back to map_pages anyway. Round
 *     to a 2 MiB span (order=9, 512 pages) covering the full 2 MiB block
 *     containing the RC. We also align the VA/PA down so map() sees a
 *     2 MiB-aligned start and picks block descriptors - actually no, the
 *     RC's natural alignment is 1 MiB so we just use page granularity
 *     (order=4 = 16 pages = 64 KiB).
 *
 *   - PCIE_MEM_WIN_CPU_PHYS (0x600000000) is 2 MiB-aligned, so order=9
 *     dispatches to map_blocks and installs a single L2 block descriptor.
 *     We over-allocate VA (2 MiB) for what's really a 64 KiB MMIO region;
 *     the RC's outbound window only forwards the first MiB so the rest
 *     just won't respond. No harm.
 */
static bool map_pcie_regions(void) {
    u64 flags = MAP_KERNEL | MAP_DEVICE | MAP_NOCACHE | MAP_READ | MAP_WRITE;

    /* RC: 16 pages = 64 KiB. order=4 -> map_pages path (small region). */
    if (!map(PCIE_RC_BASE, PCIE_RC_BASE_PHYS, 4, flags, (u64)L0_TABLE)) {
        ERROR("pcie: failed to map RC registers\n");
        return FALSE;
    }

    /* Outbound window: order=9 (2 MiB) -> 2 MiB-aligned VA, dispatches to
     * map_blocks. One L2 block descriptor covers the whole thing. */
    u64 mem_win_virt = PCIE_MEM_WIN_CPU_PHYS | 0xFFFF800000000000UL;
    if (!map(mem_win_virt, PCIE_MEM_WIN_CPU_PHYS, 9, flags, (u64)L0_TABLE)) {
        ERROR("pcie: failed to map outbound memory window\n");
        return FALSE;
    }

    /* Defensive: ensure the page-table writes are visible to the MMU
     * walker and any stale walk-cache entries are dropped. */
    asm volatile(
        "dsb ishst\n"
        "tlbi vmalle1is\n"
        "dsb ish\n"
        "isb\n"
        ::: "memory");
    return TRUE;
}


/* ----------------------------------------------------------------------- *
 * Step 2: reset dance + window configuration.
 *
 * RGR1_SW_INIT_1 has two reset bits, both asserted out of POR:
 *   bit 0 = PERST (link reset visible to the downstream device)
 *   bit 1 = INIT  (controller's own internal reset)
 *
 * We deassert INIT, keep PERST asserted while we configure the windows,
 * then deassert PERST and wait for MISC_PCIE_STATUS.LINKUP.
 * ----------------------------------------------------------------------- */

static bool brcm_pcie_setup(void) {
    wr32(RC(PCIE_RGR1_SW_INIT_1),
         PCIE_RGR1_SW_INIT_1_PERST_MASK | PCIE_RGR1_SW_INIT_1_INIT_MASK);
    timer_sleep(2);
    wr32(RC(PCIE_RGR1_SW_INIT_1), PCIE_RGR1_SW_INIT_1_PERST_MASK);
    timer_sleep(2);

    /* MISC_CTRL:
     *  - SCB_ACCESS_EN  : enable inbound memory access at all
     *  - CFG_READ_UR_MODE: make missing-device config reads return UR
     *    instead of timing out
     *  - max burst size : 128 bytes (VL805 sweet spot)
     *  - SCB0_SIZE      : log2(inbound_size) - 15. For 1 GiB that's 15. */
    u32 ctrl = rd32(RC(PCIE_MISC_MISC_CTRL));
    ctrl &= ~PCIE_MISC_MISC_CTRL_MAX_BURST_MASK;
    ctrl |=  PCIE_MISC_MISC_CTRL_BURST_SIZE_128;
    ctrl |=  PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN;
    ctrl |=  PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE;
    ctrl &= ~PCIE_MISC_MISC_CTRL_SCB0_SIZE_MASK;
    ctrl |=  (15u << PCIE_MISC_MISC_CTRL_SCB0_SIZE_SHIFT);
    wr32(RC(PCIE_MISC_MISC_CTRL), ctrl);

    /* --- Inbound (device->memory) window: RC_BAR2.
     * PCIe sees DRAM[0..1 GiB] directly at bus address 0..1 GiB.
     * Low-5 bits of the LO register encode size as log2(size) - 15. */
    wr32(RC(PCIE_MISC_RC_BAR2_CONFIG_LO),
         (u32)(PCIE_INBOUND_DRAM_BASE & ~0xfu) | 15);
    wr32(RC(PCIE_MISC_RC_BAR2_CONFIG_HI),
         (u32)(PCIE_INBOUND_DRAM_BASE >> 32));

    /* Disable RC_BAR1 and RC_BAR3 (unused on Pi 4B). */
    wr32(RC(PCIE_MISC_RC_BAR1_CONFIG_LO), 0);
    wr32(RC(PCIE_MISC_RC_BAR3_CONFIG_LO), 0);

    /* --- Outbound (CPU->device) window 0.
     *  LO         : low 32 bits of the PCIe-side base
     *  HI         : high 32 bits of the CPU-side base
     *  BASE_LIMIT : low half = base in MiB, high half = limit in MiB
     *  BASE_HI    : top 32 bits of the CPU base (for >4 GiB addresses)
     *  LIMIT_HI   : top 32 bits of the CPU limit                       */
    u64 cpu_base = PCIE_MEM_WIN_CPU_PHYS;
    u64 cpu_lim  = cpu_base + 0x100000 - 1;
    u32 pci_base = (u32)PCIE_MEM_WIN_PCI_ADDR;

    wr32(RC(PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO), pci_base);
    wr32(RC(PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI), 0);

    u32 base_limit = ((u32)(cpu_base >> 20) & 0xfff)
                   | (((u32)(cpu_lim  >> 20) & 0xfff) << 16);
    wr32(RC(PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT), base_limit);
    wr32(RC(PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI),  (u32)(cpu_base >> 32));
    wr32(RC(PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI), (u32)(cpu_lim  >> 32));

    /* Release PERST. The downstream device sees its PERSTN line rise,
     * comes out of reset, and starts link training. */
    u32 sw = rd32(RC(PCIE_RGR1_SW_INIT_1));
    sw &= ~PCIE_RGR1_SW_INIT_1_PERST_MASK;
    wr32(RC(PCIE_RGR1_SW_INIT_1), sw);

    /* Wait up to 100 ms for LINKUP. The VL805 trains in well under that. */
    for (int i = 0; i < 100; i++) {
        u32 sts = rd32(RC(PCIE_MISC_PCIE_STATUS));
        if (sts & PCIE_MISC_PCIE_STATUS_LINKUP) {
            DEBUG("pcie: link up after %d ms (status=0x%x)\n", i, sts);
            return TRUE;
        }
        timer_sleep(1);
    }

    WARNING("pcie: link did not come up (status=0x%x)\n",
            rd32(RC(PCIE_MISC_PCIE_STATUS)));
    return FALSE;
}


/* ----------------------------------------------------------------------- *
 * Set the RC's secondary/subordinate bus numbers.
 *
 * The BCM2711 PCIe RC is itself a PCI-to-PCI bridge (Type 1 header) sitting
 * at bus 0, dev 0, fn 0. Out of reset its bus number register reads 0/0/0,
 * which means "don't forward config transactions to any downstream bus" -
 * so any access to bus 1 returns Unsupported Request, surfaced to us as
 * VID:DID = 0xFFFF:0xFFFF.
 *
 * We claim bus 1 for the downstream link by writing the standard Type 1
 * Bus Number Register at offset 0x18:
 *   byte 0  (0x18) = Primary Bus Number     = 0
 *   byte 1  (0x19) = Secondary Bus Number   = 1
 *   byte 2  (0x1A) = Subordinate Bus Number = 1
 *   byte 3  (0x1B) = Secondary Latency Timer (don't care)
 *
 * Combined value: 0x00010100. Written via our own bus-0 fast path, which
 * just hits the RC's config space directly.
 * ----------------------------------------------------------------------- */
static void brcm_pcie_set_bus_numbers(void) {
    u32 v = pci_config_read32(0, 0, 0, 0x18);
    v &= ~0x00FFFFFFu;
    v |=  0x00010100u;     /* prim=0, sec=1, sub=1 */
    pci_config_write32(0, 0, 0, 0x18, v);
    DEBUG("pcie: set bus numbers (RC[0x18] = 0x%x)\n",
          pci_config_read32(0, 0, 0, 0x18));
}


/* ----------------------------------------------------------------------- *
 * Step 3: PCI config-space accessors via EXT_CFG_INDEX/DATA.
 *
 * To access (bus, dev, func, reg) for bus != 0:
 *   1. Write (bus<<20)|(dev<<15)|(func<<12)|(reg & 0xFFC) to EXT_CFG_INDEX
 *      (the spec says low 12 bits of the index must be zero, so we mask).
 *   2. Read or write EXT_CFG_DATA + (reg & 0xFFC).
 *
 * For bus == 0 we just access the RC's own config space, overlaid at
 * offset 0 of the RC register block.
 * ----------------------------------------------------------------------- */

u32 pci_config_read32(u8 bus, u8 dev, u8 func, u16 reg) {
    if (bus == 0) return rd32(RC(reg & ~0x3u));

    u32 idx = ((u32)bus << 20) | ((u32)dev << 15)
            | ((u32)func << 12) | (reg & 0xFFFu);
    wr32(RC(PCIE_EXT_CFG_INDEX), idx & ~0xFFFu);
    return rd32(RC(PCIE_EXT_CFG_DATA) + (reg & 0xFFC));
}

void pci_config_write32(u8 bus, u8 dev, u8 func, u16 reg, u32 val) {
    if (bus == 0) { wr32(RC(reg & ~0x3u), val); return; }

    u32 idx = ((u32)bus << 20) | ((u32)dev << 15)
            | ((u32)func << 12) | (reg & 0xFFFu);
    wr32(RC(PCIE_EXT_CFG_INDEX), idx & ~0xFFFu);
    wr32(RC(PCIE_EXT_CFG_DATA) + (reg & 0xFFC), val);
}

u16 pci_config_read16(u8 bus, u8 dev, u8 func, u16 reg) {
    u32 w = pci_config_read32(bus, dev, func, reg & ~0x3u);
    return (u16)((w >> ((reg & 0x2) * 8)) & 0xFFFFu);
}

u8 pci_config_read8(u8 bus, u8 dev, u8 func, u16 reg) {
    u32 w = pci_config_read32(bus, dev, func, reg & ~0x3u);
    return (u8)((w >> ((reg & 0x3) * 8)) & 0xFFu);
}

void pci_config_write16(u8 bus, u8 dev, u8 func, u16 reg, u16 val) {
    u32 w  = pci_config_read32(bus, dev, func, reg & ~0x3u);
    u32 sh = (reg & 0x2) * 8;
    u32 nw = (w & ~(0xFFFFu << sh)) | ((u32)val << sh);
    pci_config_write32(bus, dev, func, reg & ~0x3u, nw);
}

void pci_config_write8(u8 bus, u8 dev, u8 func, u16 reg, u8 val) {
    u32 w  = pci_config_read32(bus, dev, func, reg & ~0x3u);
    u32 sh = (reg & 0x3) * 8;
    u32 nw = (w & ~(0xFFu << sh)) | ((u32)val << sh);
    pci_config_write32(bus, dev, func, reg & ~0x3u, nw);
}


/* ----------------------------------------------------------------------- *
 * Step 4: locate the VL805 and program its BARs.
 * ----------------------------------------------------------------------- */

static bool find_and_setup_vl805(pcie_dev_t* out) {
    /* The VL805 sits at bus 1, dev 0, func 0 - the only PCIe device wired
     * into the Pi 4B. No need to scan, just probe. */
    u32 ids = pci_config_read32(1, 0, 0, PCI_VENDOR_ID);
    u16 vid = ids & 0xFFFF;
    u16 did = ids >> 16;
    DEBUG("pcie: bus 1 dev 0 fn 0 -> vid=0x%x did=0x%x\n", vid, did);

    if (vid != VL805_VENDOR_ID || did != VL805_DEVICE_ID) {
        WARNING("pcie: VL805 not found (got %x:%x)\n", vid, did);
        out->device_present = FALSE;
        return FALSE;
    }

    /* Program BAR0 to the start of our outbound PCIe window.
     * xHCI MMIO is ~64 KiB - well within the mapped region. */
    pci_config_write32(1, 0, 0, PCI_BAR0, (u32)PCIE_MEM_WIN_PCI_ADDR);
    pci_config_write32(1, 0, 0, PCI_BAR1, 0);   /* 64-bit BAR upper half */

    /* Enable Memory Space + Bus Master. Without these, the controller's
     * CPU-side reads and its DMA writes both fail silently. */
    u16 cmd = pci_config_read16(1, 0, 0, PCI_COMMAND);
    cmd |= PCI_COMMAND_MEM | PCI_COMMAND_MASTER;
    pci_config_write16(1, 0, 0, PCI_COMMAND, cmd);

    out->vl805_mmio_virt = PCIE_MEM_WIN_CPU_PHYS | 0xFFFF800000000000UL;
    out->vl805_mmio_size = 0x10000;
    out->bus = 1; out->dev = 0; out->func = 0;
    out->link_up = TRUE;
    out->device_present = TRUE;

    /* The page table mapping was set up earlier in map_pcie_regions().
     * What we did here is program the device-side translation: the VL805
     * now responds to PCIe bus address PCIE_MEM_WIN_PCI_ADDR, which the
     * RC's outbound window forwards from CPU physical PCIE_MEM_WIN_CPU_PHYS,
     * which our page tables map to the virtual address below. */
    INFO("pcie: VL805 BAR0 -> PCIe 0x%x; reachable at virt 0x%x\n",
         (u32)PCIE_MEM_WIN_PCI_ADDR, out->vl805_mmio_virt);

    /* Ask the GPU firmware to (re-)upload the VL805 xHCI firmware over its
     * I2C side-channel. start4.elf normally does this at boot, but we just
     * tore down the link via PERST so the device-side firmware was lost.
     * Without this call, BAR reads to the xHCI MMIO return 0xDEADxxxx
     * (Broadcom's "TLP got no completion" pattern). */
    {
        struct PACKED {
            mailbox_tag tag;
            u32         dev_addr;   /* (bus<<20)|(dev<<15)|(fn<<12) */
        } mbx;
        mbx.tag.id           = RPI_FIRMWARE_NOTIFY_XHCI_RESET;
        mbx.tag.buffer_size  = 4;
        mbx.tag.value_length = 4;
        mbx.dev_addr = ((u32)out->bus  << 20)
                     | ((u32)out->dev  << 15)
                     | ((u32)out->func << 12);

        if (!mailbox_process((mailbox_tag *)&mbx, sizeof(mbx))) {
            WARNING("pcie: NOTIFY_XHCI_RESET mailbox call failed\n");
            /* Don't bail - on some boards (rev 1.4+) start4.elf already
             * loaded the firmware and there's nothing more to do; the
             * mailbox call is harmless / no-op there. */
        } else {
            DEBUG("pcie: VL805 firmware reload requested\n");
        }
    }

    return TRUE;
}


/* ----------------------------------------------------------------------- *
 * Step 5: legacy IRQ helpers.
 *
 * PCIE_INTR2 has a bank of write-1-to-clear status bits, one per source.
 * For legacy INTA/B/C/D, the relevant bits are 15..18 of the status word.
 *
 * Register map (offsets from PCIE_INTR2_CPU_BASE):
 *   0x00 STATUS      (W1C)
 *   0x08 CLR         (W1C alias)
 *   0x0C MASK_STATUS
 *   0x10 MASK_SET    (write 1 to MASK = disable)
 *   0x14 MASK_CLR    (write 1 to UNMASK = enable)
 * ----------------------------------------------------------------------- */

#define INTR2_STATUS         (PCIE_INTR2_CPU_BASE + 0x00)
#define INTR2_CLR            (PCIE_INTR2_CPU_BASE + 0x08)
#define INTR2_MASK_CLR       (PCIE_INTR2_CPU_BASE + 0x14)

void pcie_enable_legacy_irqs(void) {
    u32 mask = PCIE_INTR2_PCI_INTA | PCIE_INTR2_PCI_INTB
             | PCIE_INTR2_PCI_INTC | PCIE_INTR2_PCI_INTD;
    wr32(RC(INTR2_CLR),      mask);    /* clear any latched bits */
    wr32(RC(INTR2_MASK_CLR), mask);    /* unmask */
}

void pcie_ack_legacy_irq(u32 intr2_status) { wr32(RC(INTR2_CLR), intr2_status); }
u32  pcie_read_intr2_status(void)         { return rd32(RC(INTR2_STATUS)); }


/* ----------------------------------------------------------------------- *
 * Public entry point.
 * ----------------------------------------------------------------------- */

bool pcie_init(pcie_dev_t* out) {
    /* The PCIe and USB power domains share one rail on the BCM2711, so
     * this single mailbox call turns both on. */
    if (!mailbox_pcie_usb_power_on()) {
        ERROR("pcie: mailbox refused to power on PCIe/USB rail\n");
        return FALSE;
    }

    if (!map_pcie_regions())   return FALSE;
    if (!brcm_pcie_setup())    return FALSE;

    /* Settling delay: let the downstream device finish its own init
     * before we start poking config space. */
    timer_sleep(10);

    u32 rev = rd32(RC(PCIE_MISC_REVISION));
    INFO("pcie: RC revision 0x%x\n", rev);

    /* The downstream bus is unreachable until we tell the RC's bridge
     * register that bus 1 lives behind it. */
    brcm_pcie_set_bus_numbers();

    if (!find_and_setup_vl805(out)) return FALSE;
    return TRUE;
}
