/*
 * Bug-finding invariants for the buddy allocator (as opposed to the
 * regression-guard checks in test_buddy.c). Expected values come from the
 * *contract*, not the implementation, and they drive the free/coalesce/edge
 * paths the guards never touched.
 *
 * Attribution: the structural walk is run after EVERY alloc/free in the churn
 * test and carries a "phase" tag, so a failure reports the exact operation
 * that broke the invariant -- and it classifies the corruption signature to
 * point at the likely function.
 *
 * Design assumptions (arbitrary-alignment, power-of-2 model):
 *   - [RESV, NPAGES) tiles into contiguous blocks; each block is 2^order pages
 *     headed by a page with PAGE_BUDDY_HEAD.
 *   - ALLOCATED blocks carry full tails (flags=TAIL, order=head pfn).
 *   - FREE blocks are head-only, so tails are checked only on non-free blocks.
 */
#include "test.h"
#include "host_env.h"
#include "memory/paging.h"

#define ULL(x) ((unsigned long long)(x))
#define MAX_TEST_ORDER 18        /* safe probe start: the buddy_lists top index.
                                    (once buddy_alloc clamps oversized requests to
                                    NULL, this can be any generous upper bound and
                                    stops needing to track MAX_ORDER) */

/* Walk [RESV, NPAGES) block by block and assert the tiling invariant.
 * `phase`/`op`/`op_pfn` describe the operation that just ran, so failures are
 * attributable; the message also guesses the culprit from the signature. */
static void check_structure(const char *phase, int op, unsigned long op_pfn) {
    page_frame_t *PFA = host_pfa();
    u64 RESV = host_reserved_pages();
    u64 pfn  = RESV;

    while (pfn < HOST_NPAGES) {
        page_frame_t *h = &PFA[pfn];

        if (!(h->flags.bits.flags & PAGE_BUDDY_HEAD)) {
            const char *hint =
                (h->flags.bits.flags == 0)
                    ? "  <- boundary page has no HEAD flag: signature of _split_down (sets .order, never .flags)"
              : (h->flags.bits.flags & PAGE_BUDDY_TAIL)
                    ? "  <- a TAIL where a head was expected: an upstream block's order/size is wrong"
                    : "";
            test_failf(__FILE__, __LINE__,
                "[%s op#%d pfn=0x%lx] pfn 0x%llx: expected block head, but flags=0x%x state=0x%x order=0x%llx%s",
                phase, op, op_pfn, ULL(pfn),
                h->flags.bits.flags, h->flags.bits.state, ULL(h->order), hint);
        }

        u64 o = h->order;
        if (o >= 63)
            test_failf(__FILE__, __LINE__,
                "[%s op#%d pfn=0x%lx] pfn 0x%llx: nonsense head order 0x%llx",
                phase, op, op_pfn, ULL(pfn), ULL(o));

        u64 span = 1ULL << o;
        if (pfn + span > HOST_NPAGES)
            test_failf(__FILE__, __LINE__,
                "[%s op#%d pfn=0x%lx] pfn 0x%llx: order-%llu block overruns region (end 0x%llx > 0x%llx)",
                phase, op, op_pfn, ULL(pfn), ULL(o), ULL(pfn + span), ULL((u64)HOST_NPAGES));

        if (h->flags.bits.state != PAGE_FREE) {          /* allocated: tails required */
            for (u64 t = 1; t < span; t++) {
                page_frame_t *tail = &PFA[pfn + t];
                if (!(tail->flags.bits.flags & PAGE_BUDDY_TAIL))
                    test_failf(__FILE__, __LINE__,
                        "[%s op#%d pfn=0x%lx] pfn 0x%llx: interior of allocated head 0x%llx is not a tail (flags=0x%x)",
                        phase, op, op_pfn, ULL(pfn + t), ULL(pfn), tail->flags.bits.flags);
                if (tail->order != pfn)
                    test_failf(__FILE__, __LINE__,
                        "[%s op#%d pfn=0x%lx] pfn 0x%llx: tail .order=0x%llx, expected head pfn 0x%llx",
                        phase, op, op_pfn, ULL(pfn + t), ULL(tail->order), ULL(pfn));
            }
        }
        pfn += span;
    }

    if (pfn != HOST_NPAGES)
        test_failf(__FILE__, __LINE__,
            "[%s op#%d pfn=0x%lx] blocks did not tile the region: ended at 0x%llx, expected 0x%llx",
            phase, op, op_pfn, ULL(pfn), ULL((u64)HOST_NPAGES));
}

/* fresh init must already satisfy the structural invariant */
TEST(invariant, structure_holds_after_init) {
    initialize_page_frame_array();
    check_structure("init", -1, 0);
}

/* alloc/free a spread of small blocks, checking the invariant after EACH op so
 * a failure names the operation (alloc -> split/alloc_and_return path;
 * free -> buddy_free/coalesce_up path) */
TEST(invariant, structure_intact_after_churn) {
    initialize_page_frame_array();

    u64 saved[64];
    int n = 0;
    u8  orders[] = {0, 1, 0, 2, 0, 1, 3, 0};

    for (int round = 0; round < 8; round++) {
        for (int k = 0; k < 8; k++) {
            u8  o  = orders[k];
            u64 va = buddy_alloc((1ULL << o) * PAGE_SIZE);
            if (!va) continue;
            u64 pfn = va_to_pa(va) >> 12;
            saved[n++] = pfn;
            check_structure("after alloc", n - 1, (unsigned long)pfn);
        }
    }
    for (int i = 0; i < n; i++) {
        buddy_free((void *)(uintptr_t)pa_to_va(saved[i] << 12));
        check_structure("after free", i, (unsigned long)saved[i]);
    }
}

/* ---- randomized stress with a double-vend detector ---------------------- */
#define MAXLIVE 2048
static unsigned char g_owner[1 << 18];      /* 1 == handed out, 0 == free */
static struct { u64 pfn; u8 order; u8 used; } g_live[MAXLIVE];

TEST(invariant, no_double_vend_stress) {
    initialize_page_frame_array();
    u64 RESV = host_reserved_pages();

    for (u64 i = 0; i < HOST_NPAGES; i++) g_owner[i] = 0;
    for (int i = 0; i < MAXLIVE; i++)      g_live[i].used = 0;

    u64 rng   = 0x9e3779b97f4a7c15ULL;      /* fixed seed -> reproducible */
    int nlive = 0;

    for (int step = 0; step < 10000; step++) {
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;   /* xorshift64 */
        int do_alloc = (nlive == 0) || (((rng & 1) != 0) && nlive < MAXLIVE);

        if (do_alloc) {
            u8  order = (u8)((rng >> 8) % 4);          /* orders 0..3 */
            u64 va    = buddy_alloc((1ULL << order) * PAGE_SIZE);
            if (!va) continue;                         /* exhaustion is fine */

            u64 pfn  = va_to_pa(va) >> 12;
            u64 span = 1ULL << order;

            ASSERT_GE(pfn, RESV);
            ASSERT_TRUE(pfn + span <= HOST_NPAGES);
            for (u64 t = 0; t < span; t++) {
                if (g_owner[pfn + t])
                    test_failf(__FILE__, __LINE__,
                        "double-vend: pfn 0x%llx already live (step %d)",
                        ULL(pfn + t), step);
                g_owner[pfn + t] = 1;
            }

            int slot = -1;
            for (int j = 0; j < MAXLIVE; j++) if (!g_live[j].used) { slot = j; break; }
            g_live[slot].used = 1; g_live[slot].pfn = pfn; g_live[slot].order = order;
            nlive++;
        } else {
            int j = (int)((rng >> 8) % MAXLIVE), scanned = 0;
            while (!g_live[j].used && scanned++ < MAXLIVE) j = (j + 1) % MAXLIVE;
            if (!g_live[j].used) continue;

            u64 pfn = g_live[j].pfn, span = 1ULL << g_live[j].order;
            for (u64 t = 0; t < span; t++) g_owner[pfn + t] = 0;
            buddy_free((void *)(uintptr_t)pa_to_va(pfn << 12));
            g_live[j].used = 0; nlive--;
        }
    }

    for (int j = 0; j < MAXLIVE; j++)
        if (g_live[j].used)
            buddy_free((void *)(uintptr_t)pa_to_va(g_live[j].pfn << 12));

    check_structure("stress final", -1, 0);
}

/* ---- fill-up / split / coalesce semantics ------------------------------- */
static u64 g_pages[1 << 18];

/* allocate every page as order-0 until empty; returns the count, fills g_pages */
static int exhaust_order0(void) {
    int n = 0;
    for (;;) {
        u64 va = buddy_alloc(PAGE_SIZE);
        if (!va) break;
        g_pages[n++] = va;
        if (n >= (int)HOST_NPAGES) break;    /* safety: never loop past capacity */
    }
    return n;
}
static void free_pages(int n) {
    for (int i = 0; i < n; i++) buddy_free((void *)(uintptr_t)g_pages[i]);
}

/* every buddy-managed page is allocatable as order-0 exactly once, then dry */
TEST(invariant, exhaustion_is_exact) {
    initialize_page_frame_array();
    u64 RESV = host_reserved_pages();
    int n = exhaust_order0();
    ASSERT_EQ((u64)n, (u64)HOST_NPAGES - RESV);   /* no lost pages, no over-vend */
    ASSERT_EQ(buddy_alloc(PAGE_SIZE), 0);         /* allocator is now dry */
    free_pages(n);
}

/* draining and refilling loses nothing */
TEST(invariant, capacity_conserved_across_cycle) {
    initialize_page_frame_array();
    int c1 = exhaust_order0(); free_pages(c1);
    int c2 = exhaust_order0(); free_pages(c2);
    ASSERT_EQ((u64)c1, (u64)c2);
}

/* largest order the allocator can currently serve (non-destructive: alloc+free) */
static u32 probe_max_order(void) {
    for (u32 o = MAX_TEST_ORDER; o > 0; o--) {
        u64 b = buddy_alloc((1ULL << o) * PAGE_SIZE);
        if (b) { buddy_free((void *)(uintptr_t)b); return o; }
    }
    return 0;
}

/*
 * Coalescing-completeness: after this returns, no free block may have a free
 * same-order buddy left un-merged. This is the real "coalescing did its job"
 * property and is independent of the region's exact shape -- so it does NOT
 * false-fail on the fact that init lays blocks at the unaligned base while
 * coalesce_up is XOR-aligned (both are valid maximal decompositions).
 */
static void assert_fully_coalesced(void) {
    page_frame_t *PFA = host_pfa();
    u64 RESV = host_reserved_pages();
    u64 pfn = RESV;
    while (pfn < HOST_NPAGES) {
        page_frame_t *h = &PFA[pfn];
        u64 o = h->order;
        if (o >= 63) return;                      /* structure test covers validity */
        u64 span = 1ULL << o;
        if (h->flags.bits.state == PAGE_FREE) {
            u64 buddy = pfn ^ span;               /* the aligned buddy */
            if (buddy >= RESV && buddy < HOST_NPAGES) {
                page_frame_t *b = &PFA[buddy];
                if (b->flags.bits.state == PAGE_FREE &&
                    (b->flags.bits.flags & PAGE_BUDDY_HEAD) &&
                    b->order == o)
                    test_failf(__FILE__, __LINE__,
                        "not fully coalesced: free order-%llu blocks 0x%llx and its "
                        "buddy 0x%llx were left un-merged", ULL(o), ULL(pfn), ULL(buddy));
            }
        }
        pfn += span;
    }
}

/*
 * THE coalescing test. Shatter the whole region into order-0 pages, free them
 * all, then demand two things: the largest block is still allocatable (capacity
 * survived), AND nothing mergeable remains (coalescing ran to completion). The
 * second check is what makes this real -- a partial coalescer that leaves an
 * order-17 block but strands mergeable pairs elsewhere passes the max-order
 * check yet fails here.
 */
TEST(invariant, full_drain_coalesces_to_max) {
    initialize_page_frame_array();

    u32 before = probe_max_order();
    ASSERT_TRUE(before > 0);

    int n = exhaust_order0();
    free_pages(n);

    u32 after = probe_max_order();
    if (after < before)
        test_failf(__FILE__, __LINE__,
            "largest block fell from order %u to %u after a full drain + free",
            before, after);

    check_structure("after full drain", -1, 0);   /* region still tiles validly */
    assert_fully_coalesced();                      /* and nothing mergeable is stranded */
}

/* direct merge: two order-1 buddies, freed, must fuse into an order-2 head --
 * and in every case nothing mergeable may be left behind */
TEST(invariant, adjacent_buddies_coalesce) {
    initialize_page_frame_array();
    u64 a = buddy_alloc(2 * PAGE_SIZE);  ASSERT_NE(a, 0);
    u64 b = buddy_alloc(2 * PAGE_SIZE);  ASSERT_NE(b, 0);
    u64 pa = va_to_pa(a) >> 12, pb = va_to_pa(b) >> 12;

    buddy_free((void *)(uintptr_t)a);
    buddy_free((void *)(uintptr_t)b);

    if ((pa ^ 2) == pb) {                          /* they were aligned order-1 buddies */
        u64 head = (pa < pb) ? pa : pb;
        page_frame_t *h = &host_pfa()[head];
        if (!(h->flags.bits.flags & PAGE_BUDDY_HEAD) ||
            h->order != 2 || h->flags.bits.state != PAGE_FREE)
            test_failf(__FILE__, __LINE__,
                "order-1 buddies 0x%llx/0x%llx did not fuse into an order-2 head "
                "(order=%llu state=%u flags=0x%x)",
                ULL(pa), ULL(pb), ULL(h->order), h->flags.bits.state, h->flags.bits.flags);
    }
    assert_fully_coalesced();
}

/* a single deep split (order-0 from a fresh large-block region) must leave the
 * whole region tiled with valid head/tail blocks -- guards _split_down's
 * flagging of the split-off halves (its own "// BUG" concern) */
TEST(invariant, split_produces_valid_heads) {
    initialize_page_frame_array();
    u64 p = buddy_alloc(PAGE_SIZE);
    ASSERT_NE(p, 0);
    u64 pfn = va_to_pa(p) >> 12;
    page_frame_t *h = &host_pfa()[pfn];
    ASSERT_TRUE(h->flags.bits.flags & PAGE_BUDDY_HEAD);
    ASSERT_EQ(h->order, 0);
    check_structure("after single split-alloc", 0, (unsigned long)pfn);
}

/* ---- robustness / adversarial inputs ------------------------------------ */

/* every buddy allocation is page-aligned (guards the contract callers rely on) */
TEST(invariant, buddy_returns_page_aligned) {
    initialize_page_frame_array();
    for (u8 o = 0; o <= 5; o++) {
        u64 v = buddy_alloc((1ULL << o) * PAGE_SIZE);
        ASSERT_NE(v, 0);
        if (v & (PAGE_SIZE - 1))
            test_failf(__FILE__, __LINE__,
                "buddy_alloc(order %u) returned 0x%llx, not page-aligned", o, ULL(v));
        buddy_free((void *)(uintptr_t)v);
    }
}

/* freeing the same page twice must not vend it twice (or corrupt the lists) */
TEST(invariant, double_free_no_double_vend) {
    initialize_page_frame_array();
    u64 p = buddy_alloc(PAGE_SIZE);
    ASSERT_NE(p, 0);
    buddy_free((void *)(uintptr_t)p);
    buddy_free((void *)(uintptr_t)p);          /* double free */

    for (u64 i = 0; i < HOST_NPAGES; i++) g_owner[i] = 0;
    for (int i = 0; i < 4096; i++) {
        u64 v = buddy_alloc(PAGE_SIZE);
        if (!v) break;
        u64 pfn = va_to_pa(v) >> 12;
        if (g_owner[pfn])
            test_failf(__FILE__, __LINE__,
                "page 0x%llx vended twice after a double-free -- free list corrupted",
                ULL(pfn));
        g_owner[pfn] = 1;
    }
}

/* ---- edge branches the happy-path tests never touched ------------------- */

/* set/get owner must agree on a TAIL page (probes set_page_owner's
 * `pfn -= order` vs get_page_owner's `pfn = order`) */
TEST(invariant, owner_roundtrip_on_tail) {
    initialize_page_frame_array();
    u64   pfn  = va_to_pa(buddy_alloc(2 * PAGE_SIZE)) >> 12;   /* order-1: head + tail */
    void *tail = (void *)(uintptr_t)pa_to_va((pfn + 1) << 12);

    set_page_owner(tail, PAGE_SLAB);
    ASSERT_EQ(get_page_owner(tail), PAGE_SLAB);
}

/* head_from_page on a non-buddy page must return NULL */
TEST(invariant, head_from_page_null_on_non_head) {
    initialize_page_frame_array();
    void *reserved = (void *)(uintptr_t)pa_to_va(0);          /* pfn 0 is RESERVED */
    ASSERT_EQ(head_from_page(reserved), (void *)0);
}
