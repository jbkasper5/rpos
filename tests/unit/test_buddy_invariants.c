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
