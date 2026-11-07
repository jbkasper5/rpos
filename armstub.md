# Armstub findings

Reference notes from the cache-enable debugging session (2026-11-06). This compares
our boot path against the stock Raspberry Pi stub,
[`raspberrypi/tools/armstubs/armstub8.S`](https://github.com/raspberrypi/tools/blob/master/armstubs/armstub8.S).

## Background

`config.txt` sets `armstub=armstub.bin`, which **replaces** the firmware's built-in stub.
Our stub (`src/armstub/armstub.S`) only does this:

```asm
ldr w1, kernel_entry32
br  x1
```

So every CPU setup step the stock stub performs either happens in `boot.S` (at EL3, on
the master core) or does not happen at all. When something low-level breaks, diff
against the stock stub first.

## Root cause of the caching bug: `L2CTLR_EL1`

The stock stub sets the Cortex-A72 L2 data RAM latency for BCM2711:

```asm
#define L2CTLR_EL1  S3_1_C11_C0_2
    mrs x0, L2CTLR_EL1
    mov x1, #0x22          // data RAM latency + setup ("latency 3")
    orr x0, x0, x1
    msr L2CTLR_EL1, x0
```

We never did this. With the default latency, **a line fetched from DRAM is correct on
the first read, but later hits in L2 return corrupted data.**

### Symptom signature (recognise it if it ever comes back)

- Everything works while nothing is cacheable. It breaks as soon as page-table walks
  (`TCR_EL1` IRGN/ORGN = WB) or RAM are made cacheable.
- **The DRAM contents are correct.** `monitor mdd phys` shows good descriptors and data,
  but the CPU sees something else.
- **A table line that walked correctly once gives garbage later.** For example, page
  `0x81000` (`pt_lo` L3[129]) walked fine at MMU-on, while page `0x82000` (L3[130], the
  same 64-byte line) later failed. The old vectors/`el1_entry` pair behaved the same way.
- Faults look like garbage descriptors:
  - **FST=7**: translation fault at L3, because bit 0 of the descriptor read as 0.
  - **FST=3**: address size fault at L3, because the "OA" had bits above the 32-bit PA
    range (`TCR.IPS=0`).
- Data structures in cacheable RAM get corrupted. This explains the earlier
  `file_t->inode` corruption.
- Cache maintenance (set/way invalidate, `dc ivac` ranges, TLBI ordering) cannot fix it.
  The month of cache-procedure research went in the wrong direction because the
  corruption is in the L2 RAM timing, not in coherency.

### Current state and plan

- **Now:** the write is at the top of `master:` in `src/kernel/src/boot.S`, the first
  thing core 0 does at EL3. This works.
- **Plan:** move it into `src/armstub/armstub.S`, before the `ldr w1, kernel_entry32`, so
  it runs before the kernel. Then remove it from `boot.S`, so there is one source of
  truth.
  - `L2CTLR` is **cluster-wide** (shared L2). The stock stub writes it from every core
    as they all run the stub, which is fine.
  - It must be written **before anything is cached**.
  - There is plenty of room: everything before `0xf0` is free. The firmware fields at
    `0xf0`, `0xf4`, `0xf8` and `0xfc` must stay at their offsets, and the binary
    stays 256 bytes.
  - `make` (the `all` target) already copies `prod/armstub.bin` to the SD card, so a
    stub change deploys with a normal build.

## Full comparison: stock armstub8 vs. us

### Can bite now, or as soon as configuration changes

| # | Stock armstub8 | Ours | Why it matters |
|---|---|---|---|
| 1 | `L2CTLR_EL1 \|= 0x22` | In `boot.S` (move to stub) | The caching bug above. |
| 2 | `SCR_EL3 = 0x5b1` (RW, HCE, SMD, RES1 bits 5 and 4, NS) | Reads the current value and ORs in RW and NS | Other bits keep their reset or firmware values. If IRQ, FIQ or EA (bits 1–3) is set, those exceptions go to EL3. `VBAR_EL3` is the high VA, which EL3 can't fetch with its MMU off, so the core hangs silently. HCE=0 makes `hvc` undefined. |
| 3 | `SPSR_EL3 = 0x3c9` (DAIF masked, EL2h) | `0b01001` (DAIF **unmasked**) | Interrupts are live during EL2. `VBAR_EL2` is also the unreachable high VA, so any IRQ or SError at EL2 hangs. |
| 4 | `SCTLR_EL2 = 0x30c50830` (all RES1 bits set) | `xzr` | Zero in RES1 bits is architecturally wrong. Harmless today, but don't rely on it. |
| 5 | (Linux sets `SCTLR_EL1`) | `0x30C00000` | Missing the v8.0 RES1 bits 11 and 20. Should be `0x30D00800`. |
| 6 | `CNTFRQ_EL0 = 54000000` | Never set | Only EL3 can write it, so it holds garbage. Anything that reads it gets the wrong frequency: musl or library code that reads the counter directly, and any future kernel code. |
| 7 | `CNTVOFF_EL2 = 0` | Never set | Its reset value is unknown, so `CNTVCT` (used by `usec_since_boot` and the virtual timer) has an arbitrary offset from `CNTPCT`. |
| 8 | `LOCAL_CONTROL = 0`, `LOCAL_PRESCALER = 0x80000000`: crystal source, +1 per tick, 54 MHz | Not set | The counter runs at the default prescaler, **2.8125 MHz**. That's why `utils.S` hard-codes `2812500`. It's consistent today, but pick one convention and make `CNTFRQ_EL0` match. Switching to 54 MHz means updating `2812500` and `QUANTUM`. |
| 9 | `CPTR_EL3 = 0` | Only clears the FP/SIMD trap bit (bit 10) | TCPAC (bit 31) and TTA (bit 20) keep their reset values. If either is set, `cpacr_el1` or trace accesses trap to the unreachable EL3. |

### Matters for tuning later

**`ACTLR_EL3 = 0x73`** (bits 0, 1, 4, 5, 6) is set by the stock stub and never by us.
Without it, EL2 and EL1 can't access the implementation-defined control registers that
hold the cache, prefetch and coherency settings:

| Bit | Register |
|---|---|
| 0 | `CPUACTLR_EL1` |
| 1 | `CPUECTLR_EL1` |
| 4 | `L2CTLR_EL1` |
| 5 | `L2ECTLR_EL1` |
| 6 | `L2ACTLR_EL1` |

Accesses from lower ELs trap to EL3 and hang, for the same `VBAR_EL3` reason. For EL1
access, **`ACTLR_EL2 = 0x73`** (written at EL2) is needed too. If all tuning happens at
EL3 (the stub or `boot.S` before the `eret`), neither is needed.

`src/kernel/src/boot/sysregs_dbg.c` reads `CPUECTLR` (`S3_1_c15_c2_1`). If that ever runs
at EL1, this is the trap it will hit.

Neither the stock stub nor ours applies any **Cortex-A72 errata workarounds**. If you
start changing prefetch or `CPUACTLR` settings, Trusted Firmware-A's
`lib/cpus/aarch64/cortex_a72.S` is the reference for which workarounds apply to the
Pi 4's A72 revision.

### Not a problem today, but needed for SMP

- **Device tree pointer.** The firmware writes the DTB address into the stub at `0xf8`
  (`dtb_ptr32`), and the stock stub loads it into `x0` before jumping. Our stub has no
  `0xf8` field and doesn't set `x0`, so the `mov x19, x0` in `_start` saves garbage.
  This is fine for now, because the kernel hard-codes `DEVICE_TREE_BASE` to match
  `device_tree_address=0x10000` in `config.txt`.
- **Secondary cores.** The stock stub drops them to EL2 and parks them in a `wfe` loop
  on the `spin_cpu0`–`spin_cpu3` slots (`0xd8`–`0xf0`). The kernel releases a core by
  writing an entry address to its slot and issuing `sev`. Ours sends all four cores
  into `_start` at EL3, and cores 1–3 busy-loop in `_infinite` with nothing set up.
- **Per-core setup for SMP.** Each core must do its own setup at EL3 before enabling its
  caches and MMU:
  - set SMPEN in its own `CPUECTLR` (it's per core; we only set it on core 0);
  - set `SCR_EL3`, `CPTR_EL3`, `ACTLR_EL3` and `CNTVOFF_EL2` (per-core registers);
  - set `GICD_IGROUPR0` (banked per core, covers SGIs and PPIs 0–31) and the GICC
    registers.

  Only `L2CTLR` is shared across the cluster.
- **GIC.** `GICC_CTLR` is `0x3` versus the stock `0x1e7`, which adds AckCtl and the
  bypass-disable bits. That's cosmetic.

## Things in `boot.S` that conflict with adopting stock values

If you adopt the stock values above, these existing lines need replacing, not just
adding to:

- the `SCR_EL3` read-modify-write: write the full value instead;
- the `CPTR_EL3` `bic`: write `xzr` instead;
- `msr SCTLR_EL2, xzr`: write `0x30c50830` instead;
- `SPSR_EL3 = 0b01001`: use `0x3c9`;
- `SCTLR_EL1` at `el2_entry`: add bits 11 and 20 (`0x30D00800`).

## Debugging technique that found it

GDB can't read system registers on this setup, so the CPU had to copy them into GPRs
itself:

1. A 2 KiB-aligned **capture vector table** in `.text.boot`, where each slot does
   `mrs x10, esr_el1; mrs x11, far_el1; mrs x12, elr_el1; b .`.
2. Identity-map its page in `pt_lo`, and point `VBAR_EL1` at it in `enable_mmu` before
   `msr sctlr_el1`.
3. Power-cycle, run with no `monitor mdd/mdw phys` (they toggle the MMU and desync
   OpenOCD), press `^C` at the hang, then `p/x $x10 $x11 $x12`.

**Gotcha:** the table and its handler must be in a page that is **already in the TLB**
(walked at MMU-on, like the `enable_mmu` page). If they're in an unwalked page, the
broken walker can't fetch them and the core loops at `VBAR+0x200`. A 2 KiB `.align`
also aligns the whole `.text.boot` input section from that file, and link order follows
the Makefile's `find` order, so check placement with `nm` after every move.

Decoding the first capture, `ESR=0x96000047`:

| Field | Value | Meaning |
|---|---|---|
| EC | `0x25` | data abort at the same EL |
| WnR | 1 | the access was a store |
| DFSC | `0x07` | translation fault at L3 |

That proved the walker read an invalid L3 descriptor that was valid in DRAM, which
pointed at the cache hierarchy rather than the page-table code.
