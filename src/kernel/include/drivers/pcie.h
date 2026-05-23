#ifndef __DRIVERS_PCIE_H__
#define __DRIVERS_PCIE_H__

/*
 * BCM2711 PCIe Root Complex + VL805 xHCI controller driver
 * =========================================================
 *
 * The Pi 4B has exactly ONE PCIe Root Complex (RC) baked into the SoC, and
 * exactly ONE device sitting downstream of it: the VIA Labs VL805, a 4-port
 * USB 3.0 (xHCI) host controller. Every USB-A port on the board is wired to
 * the VL805. The USB-C port is NOT on this controller; it is wired to the
 * SoC's internal DWC2 (USB 2.0 OTG), which we are deliberately not using
 * because that port is being used to power the board.
 *
 * Layout in CPU physical memory (Pi 4B, BCM2711):
 *
 *   PCIE_RC_BASE_PHYS    = 0xFD500000       // 0x9310 bytes of registers
 *   PCIE_MEM_WIN_PHYS    = 0x600000000      // 1 GiB outbound window for BARs
 *   PCIE_MEM_WIN_PCI     = 0x0F8000000      // ...as seen from the PCIe bus
 *   PCIE_MEM_WIN_SIZE    = 0x40000000
 *
 * Layout once we map both regions with MAP_DEVICE through your mmu/mmap layer:
 *
 *   PCIE_RC_BASE         = 0xFFFF8000FD500000
 *   PCIE_MEM_WIN_VIRT    = (allocated by pcie_init, returned in pcie_dev_t)
 *
 * The MEM window is the only way the CPU can talk to PCIe BARs. When we
 * program the VL805's BAR0 to (say) 0x0F8000000, the controller maps that
 * PCIe bus address to CPU physical 0x600000000, which we then map to a
 * virtual address. Reads/writes to that virtual address get translated by
 * the RC into TLPs on the PCIe link and routed to the VL805.
 *
 * Inbound (device->memory) direction: the RC has a separate inbound window
 * configured via RC_BAR2. We program it to expose the first 1 GiB of DRAM
 * 1:1 to PCIe devices, so the VL805 can DMA into any kmalloc'd buffer in
 * the low 1 GiB. If you later raise this limit, update PCIE_DMA_MASK_BITS.
 */

#include "macros.h"

/* ---- Physical and virtual base addresses --------------------------------- */

#define PCIE_RC_BASE_PHYS        0xFD500000UL
#define PCIE_RC_BASE             (PCIE_RC_BASE_PHYS | 0xFFFF800000000000UL)
#define PCIE_RC_SIZE             0x10000UL          /* 64 KiB - mapped as 1 block */

#define PCIE_MEM_WIN_CPU_PHYS    0x600000000UL      /* outbound: CPU side  */
#define PCIE_MEM_WIN_PCI_ADDR    0x0F8000000UL      /* outbound: PCIe side */
#define PCIE_MEM_WIN_SIZE        0x40000000UL       /* 1 GiB */

#define PCIE_INBOUND_BASE        0x0UL              /* inbound: PCIe sees this... */
#define PCIE_INBOUND_DRAM_BASE   0x0UL              /* ...mapped to this DRAM */
#define PCIE_INBOUND_SIZE        0x40000000UL       /* 1 GiB - DMA-safe region */

/* Any DMA buffer we hand to the VL805 must have a physical address strictly
 * less than this value. kmalloc currently has no awareness of this, which
 * is the single biggest correctness concern in this driver. See xhci.c. */
#define PCIE_DMA_MASK            0x40000000UL


/* ---- BCM2711 PCIe Root Complex registers --------------------------------- *
 *
 * Sourced from the Linux kernel's drivers/pci/controller/pcie-brcmstb.c.
 * Broadcom never published a public datasheet for this block, so Linux is
 * the de-facto specification. Offsets are from PCIE_RC_BASE.
 */
#define PCIE_RC_CFG_VENDOR_VENDOR_ID         0x0000
#define PCIE_RC_CFG_PRIV1_ID_VAL3            0x043c
#define PCIE_RC_CFG_PRIV1_LINK_CAPABILITY    0x04dc
#define PCIE_RC_DL_MDIO_ADDR                 0x1100
#define PCIE_RC_DL_MDIO_WR_DATA              0x1104
#define PCIE_RC_DL_MDIO_RD_DATA              0x1108
#define PCIE_MISC_MISC_CTRL                  0x4008
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO     0x400c
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI     0x4010
#define PCIE_MISC_RC_BAR1_CONFIG_LO          0x402c
#define PCIE_MISC_RC_BAR2_CONFIG_LO          0x4034
#define PCIE_MISC_RC_BAR2_CONFIG_HI          0x4038
#define PCIE_MISC_RC_BAR3_CONFIG_LO          0x403c
#define PCIE_MISC_MSI_BAR_CONFIG_LO          0x4044
#define PCIE_MISC_MSI_BAR_CONFIG_HI          0x4048
#define PCIE_MISC_MSI_DATA_CONFIG            0x404c
#define PCIE_MISC_PCIE_CTRL                  0x4064
#define PCIE_MISC_PCIE_STATUS                0x4068
#define PCIE_MISC_REVISION                   0x406c
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT 0x4070
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI    0x4080
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI   0x4084
#define PCIE_MISC_HARD_PCIE_HARD_DEBUG       0x4204
#define PCIE_INTR2_CPU_BASE                  0x4300   /* legacy IRQ block */
#define PCIE_MSI_INTR2_BASE                  0x4500
#define PCIE_EXT_CFG_DATA                    0x8000   /* read/write through window */
#define PCIE_EXT_CFG_INDEX                   0x9000   /* select bus/dev/func/reg */
#define PCIE_RGR1_SW_INIT_1                  0x9210

/* Fields and bits we actually touch */
#define PCIE_RGR1_SW_INIT_1_PERST_MASK       0x1
#define PCIE_RGR1_SW_INIT_1_INIT_MASK        0x2

#define PCIE_MISC_PCIE_CTRL_PCIE_PERSTB      (1u << 2)

#define PCIE_MISC_MISC_CTRL_BURST_SIZE_128   (0u << 20)
#define PCIE_MISC_MISC_CTRL_MAX_BURST_MASK   (3u << 20)
#define PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN    (1u << 12)
#define PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE (1u << 13)
#define PCIE_MISC_MISC_CTRL_SCB0_SIZE_SHIFT  27
#define PCIE_MISC_MISC_CTRL_SCB0_SIZE_MASK   (0x1f << 27)

#define PCIE_MISC_PCIE_STATUS_LINKUP         (1u << 4)
#define PCIE_MISC_PCIE_STATUS_NEG_SPEED      (1u << 0)

#define PCIE_INTR2_PCI_INTA                  (1u << 15)
#define PCIE_INTR2_PCI_INTB                  (1u << 16)
#define PCIE_INTR2_PCI_INTC                  (1u << 17)
#define PCIE_INTR2_PCI_INTD                  (1u << 18)


/* ---- Standard PCI config space (Type 0) offsets -------------------------- */

#define PCI_VENDOR_ID            0x00
#define PCI_DEVICE_ID            0x02
#define PCI_COMMAND              0x04
#define PCI_STATUS               0x06
#define PCI_REVISION_ID          0x08
#define PCI_CLASS_CODE           0x09       /* 3 bytes: prog_if, subclass, class */
#define PCI_CACHE_LINE_SIZE      0x0C
#define PCI_LATENCY_TIMER        0x0D
#define PCI_HEADER_TYPE          0x0E
#define PCI_BAR0                 0x10
#define PCI_BAR1                 0x14
#define PCI_BAR2                 0x18
#define PCI_BAR3                 0x1C
#define PCI_BAR4                 0x20
#define PCI_BAR5                 0x24
#define PCI_INTERRUPT_LINE       0x3C
#define PCI_INTERRUPT_PIN        0x3D

#define PCI_COMMAND_IO           0x0001
#define PCI_COMMAND_MEM          0x0002
#define PCI_COMMAND_MASTER       0x0004

/* VIA VL805 IDs (the device we expect to find on bus 1, dev 0, func 0) */
#define VL805_VENDOR_ID          0x1106
#define VL805_DEVICE_ID          0x3483


/* ---- GIC SPIs raised by the PCIe block ----------------------------------- */

/* SPI numbers from the BCM2711 device tree. GIC INTID = SPI + 32. */
#define PCIE_INTID_LEGACY        (147 + 32)   /* = 179 */
#define PCIE_INTID_MSI           (148 + 32)   /* = 180 - unused, we use legacy */


/* ---- Public API ---------------------------------------------------------- */

/*
 * Describes the result of a successful PCIe bring-up and VL805 enumeration.
 * Filled in by pcie_init() and consumed by xhci_init().
 */
typedef struct {
    /* Virtual address where the VL805's BAR0 (xHCI MMIO) is mapped. */
    u64  vl805_mmio_virt;
    u32  vl805_mmio_size;

    /* VL805 PCIe bus / device / function (always 1/0/0 on Pi 4B, but
     * stored anyway in case anyone ever bridges another device). */
    u8   bus, dev, func;

    bool link_up;
    bool device_present;
} pcie_dev_t;


/* Brings up the BCM2711 PCIe RC, configures windows, enumerates the link,
 * locates and configures the VL805, and maps its BAR0 into kernel virtual
 * address space. On success returns TRUE and fills *out. */
bool pcie_init(pcie_dev_t* out);

/* PCI config-space helpers. They use the BCM2711 EXT_CFG_INDEX/DATA window,
 * so they work for any bus/dev/func behind the RC, not just bus 0. */
u32  pci_config_read32 (u8 bus, u8 dev, u8 func, u16 reg);
u16  pci_config_read16 (u8 bus, u8 dev, u8 func, u16 reg);
u8   pci_config_read8  (u8 bus, u8 dev, u8 func, u16 reg);
void pci_config_write32(u8 bus, u8 dev, u8 func, u16 reg, u32 val);
void pci_config_write16(u8 bus, u8 dev, u8 func, u16 reg, u16 val);
void pci_config_write8 (u8 bus, u8 dev, u8 func, u16 reg, u8  val);

/* Unmask PCIe legacy IRQs (INTA-D) at the RC. Called by xhci once it has
 * registered its handler. */
void pcie_enable_legacy_irqs(void);
void pcie_ack_legacy_irq(u32 intr2_status);
u32  pcie_read_intr2_status(void);

#endif
