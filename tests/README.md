# rpos host test harness

Runs selected **kernel memory code natively on the host** (your Mac) against a
simulated-RAM environment, so allocator/PFA logic can be unit-tested in
milliseconds instead of round-tripping through UART/JTAG on real hardware.

It is meant as a **deployment gate**: `make test` exits non-zero if any
invariant is violated, so it drops cleanly into a pre-flash / pre-commit check.

```sh
make test                 # build + run everything (exit 0 == gate passes)
make test FILTER=buddy    # run only tests whose suite/name contains "buddy"
make build                # compile only
make clean
HOST_KLOG=1 make test     # also surface the kernel's kprintf() output
```

## Guarantees / ground rules

- **No kernel source is modified.** The harness *compiles* unmodified kernel
  translation units (currently `src/kernel/src/memory/paging.c`) and links them
  against host stubs. If a kernel change is ever required, that's surfaced as a
  build failure to hand back, not patched here.
- If `paging.c` doesn't compile, neither does the harness — that's the gate
  catching a broken tree before it ships (it already caught one WIP syntax error
  during bring-up).

## How it works

A **physical address is modeled as a byte offset into one lazily-backed 1 GiB
host buffer** (`host/host_sys.c` → `aligned_alloc`). The kernel's translation
functions become trivial:

```
pa_to_va(pa) = ram + pa        va_to_pa(va) = va - ram
```

Because the allocator is internally consistent about PA↔VA, this makes all of
its pointer arithmetic land in real host memory. The stubs also model a
reserved prefix (low RAM + the frame_metadata array + a fake "test section")
so the buddy allocator behaves as it does on-target.

### Layout of `tests/`

| path | role |
|------|------|
| `framework/test.h` `.c` | tiny xUnit runner (`TEST`, `ASSERT_*`, self-registration, exit-code gate) |
| `host/host_sys.c`       | the **only** TU that includes libc; owns allocation, `panic`, `kprintf` |
| `host/host_env.c` `.h`  | address model + linker/asm-symbol stubs (kernel headers only) |
| `unit/test_pfa.c`       | page-frame-array init / reserved-boundary invariants |
| `unit/test_buddy.c`     | buddy allocator head/tail/order/no-overlap invariants |

### Two compile quirks (host-only, not kernel bugs)

- **Relaxed diagnostics for kernel TUs.** `paging.c` freely converts
  `void*`↔`u64`↔`pte*`, which the on-target aarch64 toolchain accepts but modern
  clang promotes to hard errors. `KCFLAGS` downgrades those to warnings for
  kernel sources only; harness code stays strict.
- **`pid_t` split.** The kernel's `scalar.h` does `typedef u16 pid_t`, which
  collides with the host libc's `pid_t`. Rather than hack the SDK, no single TU
  includes both universes: `host_sys.c` is libc-only, everything else is
  kernel-only, and `test.h` pulls in no system headers.

## Coverage today

`paging.c`: `initialize_page_frame_array`, `buddy_alloc`, `buddy_free`,
`get_block_order`, `get/set_page_owner`, `head_from_page`.

- `test_pfa.c` / `test_buddy.c` — **regression guards**: reserved marking,
  head/tail state+flags, order rounding, no-overlap-on-alloc.
- `test_buddy_invariants.c` — **bug finders**: expected values come from the
  contract, not the implementation, and they drive the free/coalesce/edge
  paths the guards never touched:
  - `walk_and_check_structure` — the region tiles into blocks; every head has
    `PAGE_BUDDY_HEAD` and a sane order; allocated blocks carry full tails;
    blocks tile `[RESV, NPAGES)` exactly (page conservation).
  - `no_double_vend_stress` — 10k randomized alloc/free ops with a per-page
    owner map; asserts no allocation ever overlaps a live one.
  - `owner_roundtrip_on_tail`, `head_from_page_null_on_non_head` — the tail /
    non-head edge branches the happy-path tests skipped.

## Extending

- **More buddy/PFA tests:** add `TEST(suite, name){...}` to a file in `unit/`;
  it self-registers, no wiring.
- **map / clone / teardown (cut 2):** these *dereference* the addresses they
  build, which already works because RAM is a real buffer. Add `mmap.c` to the
  kernel-TU list in the `Makefile` (replacing the no-op `map` stub in
  `host_env.c`) and the page-table walker becomes testable — enabling a
  fork→exec→exit **leak detector** (assert free-page count returns to baseline).

## Open findings (red)

The bug-finder suite currently fails on two real issues (the gate is *supposed*
to be red until they're fixed — that's the point):

- **`set_page_owner`** resolves a tail with `pfn -= frame[pfn].order` while
  `get_page_owner` uses `pfn = frame[pfn].order`. They disagree, so
  `owner_roundtrip_on_tail` fails. Fix: make `set` match `get`.
- **`_split_down`** sets `.order` on both split halves but never sets
  `.flags = PAGE_BUDDY_HEAD` (the `// BUG` at paging.c:27), so split-off free
  blocks have no head flag. `coalesce_up` requires `flags & PAGE_BUDDY_HEAD` to
  merge, so those blocks never coalesce — and the structural walk catches the
  unflagged head. Fix: stamp `flags=HEAD`, `state=FREE` on both halves.

Note: `no_double_vend_stress` ran 10k ops with **no double-vend** — the
allocation path is memory-safe; these are metadata-bookkeeping bugs, not
overlapping-memory corruption. And `head_from_page`'s `|`→`&` is already fixed
(its edge test passes and guards against regression).
