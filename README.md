# rpos

A small operating system for the **Raspberry Pi 4 Model B** (Broadcom BCM2711,
quad-core Cortex-A72, ARMv8-A / AArch64), written from scratch in C and AArch64
assembly — no external kernel, no libc. It boots on real hardware and under
QEMU.

It began as a hardware-bring-up exercise and has grown into a real kernel: a
higher-half virtual-memory system, physical and object allocators, processes
with ELF loading, and a driver stack for the Pi's peripherals. It's a learning
project and actively developed — some paths (SMP, parts of teardown/COW, a libc
userspace) are still in progress.

## What it does

**Boot & CPU bring-up**
- `armstub` + boot assembly bring the cores up, drop from EL3 to EL1, set up the
  stack and BSS, install exception vectors, and enable the MMU.
- **Higher-half kernel** — the kernel runs at a high virtual base
  (`0xFFFF800000000000`) via `TTBR1`, with per-process user space in `TTBR0`.

**Memory management** (`src/kernel/src/memory`)
- **Buddy page allocator** over a page-frame array describing physical RAM
  (split/coalesce, order-based free lists, a reserved boot prefix).
- **Slab allocator** (`kmalloc`/`kfree`) layered on the buddy for small objects,
  with per-size caches; large requests fall through to the page allocator.
- **4-level AArch64 virtual memory** — `map`/`map_pages`/`map_blocks` build page
  tables; address-space **clone with copy-on-write**, **teardown/reap**, and
  page **refcounting**.
- A **memory profiler** that reads the page-frame array to visualize usage.

**Processes & scheduling** (`src/kernel/src/system`)
- Process control blocks, a round-robin run-queue scheduler with idle/progenitor
  bootstrap, `fork`/`exec`/`exit`, syscalls, and trap-frame save/restore.
- **ELF loader** that maps program segments and a user stack for on-disk
  binaries.

**Synchronization** (`src/kernel/src/synchronization`)
- Spinlocks, mutexes, and LL/SC atomics (`ldaxr`/`stlxr`) on cacheable memory.

**Drivers & I/O** (`src/kernel/src/io`, `drivers`, `mailbox`)
- mini-UART with a `kprintf` (ANSI-aware), GPIO, framebuffer/LCD via the VideoCore
  mailbox, GIC interrupt controller, and the system timer.

**Storage & filesystem** (`src/kernel/src/emmc`, `filesystem`, `filedescriptors`)
- An eMMC/SD-card driver, an ext4 filesystem reader, file descriptors, and
  loadable user programs (e.g. `ls`, `pwd`).

**Utilities** (`src/kernel/src/utils`)
- Reusable intrusive data structures (linked lists, priority queues, tries),
  string/assembly helpers, and linker-symbol accessors.

## Testing

`tests/` is a **host test harness**: it compiles the kernel's memory allocators
(buddy + slab) *unmodified* on the host against a simulated-RAM environment and
runs a fast unit / stress / invariant / adversarial suite (`make test`) — no
hardware round-trip. It has crash isolation, a per-test watchdog, and a
`-DDEBUG` syntax gate. **CI** (`.github/workflows/memtests.yml`) runs it on every
push and pull request. See [`tests/README.md`](tests/README.md).

## Building & running

Requires an AArch64 bare-metal toolchain (`aarch64-none-elf-gcc`, `ld`,
`objcopy`).

```sh
make local        # build prod/kernel8-pi4.img + prod/armstub.bin
make              # build, then copy the image to a mounted SD card and eject
make debug        # clean rebuild with DEBUG=1 (kprintf logging + symbols)
make asm          # disassemble the kernel to objdump.S
make clean
```

Serial console over UART:

```sh
screen -L /dev/tty.usbserial-0001 115200
```

For on-hardware JTAG (OpenOCD + J-Link) and QEMU + GDB debugging, plus an AArch64
instruction/register quick-reference, see [`notes.md`](notes.md).

## Layout

```
src/kernel/        the kernel
  src/{boot,memory,system,synchronization,io,drivers,mailbox,
       emmc,filesystem,filedescriptors,utils}/
  include/         headers, incl. types/ (shared type definitions)
  linker.ld        memory map (phys base, high-half, reserved regions)
src/armstub/       EL/boot stub for the Pi 4
tests/             host test harness for the memory allocators (+ CI)
scripts/, uscripts/  SD mount/eject, OpenOCD/GDB helpers
notes.md           hardware bring-up + debugging + ARM reference notes
```
