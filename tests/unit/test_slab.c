/*
 * Slab allocator (kmalloc/kfree) invariants. Layers on top of the buddy
 * allocator, which the harness already compiles: a slab gets its backing pages
 * from buddy_alloc, and kfree routes back to buddy_free for large allocations.
 *
 * The `map` no-op stub is fine here: pa_to_va already hands back real host
 * memory, so a slab's bitmap and its objects are backed by g_ram regardless of
 * mapping.
 */
#include "test.h"
#include "host_env.h"
#include "memory/kmalloc.h"        /* kmalloc, kfree, kheap_init, CACHES */
#include "memory/paging.h"         /* get_page_owner, PAGE_SLAB/BUDDY/FREE */

#define ULL(x) ((unsigned long long)(x))

static void slab_boot(void) {
    initialize_page_frame_array();
    kheap_init();
}

static page_state owner_of(void *obj) {
    void *page = (void *)(uintptr_t)ALIGN_DOWN((uintptr_t)obj, PAGE_SIZE);
    return get_page_owner(page);
}

/* ---- kcaches (slab list) inspection ------------------------------------- *
 * These walk the real cache lists so we test the bookkeeping, not just bytes. */
static slab *slab_of(void *obj) {
    return (slab *)head_from_page(
        (void *)(uintptr_t)ALIGN_DOWN((uintptr_t)obj, PAGE_SIZE));
}
static int slab_on_list(list_head_t *head, slab *s) {
    int guard = 0;
    for (list_head_t *n = head->next; n != head; n = n->next)
        if (++guard > 4096 || list_entry(n, slab, list) == s) return 1;
    return 0;
}
static int on_any_partial(slab *s) {
    for (int i = 0; i < CACHES; i++)
        if (slab_on_list(&kcaches[i].partial_slabs, s)) return 1;
    return 0;
}
static int on_any_full(slab *s) {
    for (int i = 0; i < CACHES; i++)
        if (slab_on_list(&kcaches[i].full_slabs, s)) return 1;
    return 0;
}

/* a small (<= 2048B) allocation lives in a slab-owned page */
TEST(slab, small_alloc_owned_by_slab) {
    slab_boot();
    void *p = kmalloc(64);
    ASSERT_NE(p, 0);
    ASSERT_EQ(owner_of(p), PAGE_SLAB);
}

/* a large (> 2048B) allocation bypasses the slab and comes straight from buddy */
TEST(slab, large_alloc_bypasses_slab) {
    slab_boot();
    void *p = kmalloc(4096);
    ASSERT_NE(p, 0);
    ASSERT_EQ(owner_of(p), PAGE_BUDDY);
}

/* every live object is distinct and at least one item-size apart */
TEST(slab, objects_distinct_and_spaced) {
    slab_boot();
    uintptr_t o[20];
    for (int i = 0; i < 20; i++) {
        void *p = kmalloc(32);
        ASSERT_NE(p, 0);
        o[i] = (uintptr_t)p;
    }
    for (int i = 0; i < 20; i++)
        for (int j = i + 1; j < 20; j++) {
            uintptr_t d = (o[i] > o[j]) ? o[i] - o[j] : o[j] - o[i];
            if (d < 32)
                test_failf(__FILE__, __LINE__,
                    "objects %d (0x%llx) and %d (0x%llx) overlap: %llu bytes apart (< 32)",
                    i, ULL(o[i]), j, ULL(o[j]), ULL(d));
        }
}

/* freeing one object must not let a later alloc collide with still-live ones */
TEST(slab, free_then_alloc_no_collision) {
    slab_boot();
    void *a = kmalloc(32); ASSERT_NE(a, 0);
    void *b = kmalloc(32); ASSERT_NE(b, 0);
    void *c = kmalloc(32); ASSERT_NE(c, 0);
    kfree(b);                                 /* free the middle one */
    void *d = kmalloc(32); ASSERT_NE(d, 0);
    ASSERT_NE(d, a);                           /* must not land on a live object */
    ASSERT_NE(d, c);
}

/* churn: many random alloc/free of one class, asserting live objects never
 * overlap and freed slots are reused (kept under one slab's capacity so this
 * exercises intra-slab bitmap alloc/free, not the missing full-slab path) */
TEST(slab, object_churn_no_overlap) {
    slab_boot();
    uintptr_t live[32];
    int used[32] = {0};
    int allocs = 0, frees = 0;
    u64 rng = 0xda3e39cb94b95bdbULL;

    for (int step = 0; step < 5000; step++) {
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;

        int nlive = 0;
        for (int i = 0; i < 32; i++) nlive += used[i];
        int do_alloc = (nlive == 0) || (((rng & 1) != 0) && nlive < 30);

        if (do_alloc) {
            void *p = kmalloc(64);
            if (!p)
                test_failf(__FILE__, __LINE__,
                    "kmalloc(64) -> NULL at step=%d (nlive=%d, allocs=%d, frees=%d); "
                    "buddy still has memory, so this is corruption not exhaustion "
                    "(set a breakpoint conditional on step==%d)",
                    step, nlive, allocs, frees, step);
            allocs++;
            uintptr_t a = (uintptr_t)p;
            for (int i = 0; i < 32; i++)
                if (used[i]) {
                    uintptr_t b = live[i];
                    uintptr_t d = (a > b) ? a - b : b - a;
                    if (d < 64)
                        test_failf(__FILE__, __LINE__,
                            "live objects overlap at step=%d (allocs=%d): 0x%llx vs 0x%llx (%llu < 64)",
                            step, allocs, ULL(a), ULL(b), ULL(d));
                }
            for (int i = 0; i < 32; i++)
                if (!used[i]) { used[i] = 1; live[i] = a; break; }
        } else {
            int pick = (int)((rng >> 8) % 32), scanned = 0;
            while (!used[pick] && scanned++ < 32) pick = (pick + 1) % 32;
            if (!used[pick]) continue;
            kfree((void *)live[pick]);
            used[pick] = 0;
            frees++;
        }
    }
}

/* when the last object in a slab is freed, its pages must return to the buddy */
TEST(slab, empty_slab_returns_to_buddy) {
    slab_boot();
    void *p = kmalloc(32);
    ASSERT_NE(p, 0);
    ASSERT_EQ(owner_of(p), PAGE_SLAB);         /* sanity: it is a slab page */

    kfree(p);                                  /* last (only) object -> slab empty */

    if (owner_of(p) == PAGE_SLAB)
        test_failf(__FILE__, __LINE__,
            "empty slab page still owned by slab after final kfree "
            "-- reclaim never buddy_frees the backing pages (kmalloc.c:137)");
}

/* once the first slab of a class fills, kmalloc must allocate another one --
 * there is ~1 GiB of buddy RAM free, so NULL here means missing full-slab handling */
TEST(slab, class_survives_first_slab_filling) {
    slab_boot();
    for (int i = 0; i < 12; i++) {
        void *p = kmalloc(2048);
        if (!p)
            test_failf(__FILE__, __LINE__,
                "kmalloc(2048) #%d returned NULL with buddy memory still free "
                "-- no fresh slab when the first one filled (full_slabs never used)", i);
    }
}

/* ---- robustness / adversarial inputs ------------------------------------ */

/* kmalloc returns objects aligned to their (power-of-2) size */
TEST(slab, objects_naturally_aligned) {
    slab_boot();
    size_t sizes[] = {32, 64, 128, 256, 512, 1024, 2048};
    for (int i = 0; i < 7; i++) {
        void *p = kmalloc(sizes[i]);
        ASSERT_NE(p, 0);
        if ((uintptr_t)p & (sizes[i] - 1))
            test_failf(__FILE__, __LINE__,
                "kmalloc(%zu) returned 0x%llx, not %zu-aligned",
                sizes[i], ULL((uintptr_t)p), sizes[i]);
    }
}

/* kfree(NULL) must be a harmless no-op (Linux semantics) */
TEST(slab, kfree_null_is_noop) {
    slab_boot();
    kfree(NULL);          /* must not panic or crash */
}

/* kmalloc(0) must return a usable, freeable object rather than misbehave */
TEST(slab, kmalloc_zero_is_safe) {
    slab_boot();
    void *p = kmalloc(0);
    ASSERT_NE(p, 0);
    kfree(p);
}

/* freeing the same object twice must not underflow the slab's inuse counter */
TEST(slab, double_free_does_not_underflow_inuse) {
    slab_boot();
    void *a = kmalloc(64);
    ASSERT_NE(a, 0);
    slab *s = (slab *)head_from_page(
        (void *)(uintptr_t)ALIGN_DOWN((uintptr_t)a, PAGE_SIZE));
    kfree(a);             /* inuse -> 0 */
    kfree(a);             /* double free: inuse-- underflows to 0xFFFF */
    if (s->inuse != 0)
        test_failf(__FILE__, __LINE__,
            "slab inuse=%u after double-free (underflowed) -- kfree does not "
            "detect an already-free object", s->inuse);
}

/* an impossible size must be rejected, not spun on. kmalloc's size-rounding
 * (`while (aligned < bytes) aligned <<= 1`) overflows to 0 and loops forever for
 * huge requests -- the short watchdog turns that hang into a clean failure. */
TEST(slab, huge_alloc_does_not_hang) {
    slab_boot();
    test_set_timeout(2);                 /* expected to hang today */
    void *p = kmalloc((size_t)-1);
    ASSERT_EQ(p, (void *)0);             /* a sane allocator returns NULL */
}

/* a class must hand out exactly `total` objects from one slab, and `total` must
 * fit the 128-bit bitmap (`_addr_from_slab` indexes bitmap[i/8] up to total) */
TEST(slab, fills_slab_to_capacity) {
    slab_boot();
    void *first = kmalloc(32);
    ASSERT_NE(first, 0);
    slab *s = (slab *)head_from_page(
        (void *)(uintptr_t)ALIGN_DOWN((uintptr_t)first, PAGE_SIZE));

    u16 total = s->total;
    if (total == 0 || total > 128)
        test_failf(__FILE__, __LINE__,
            "slab total=%u does not fit the 128-bit bitmap (or is zero)", total);

    int got = 1;                          /* already have `first` */
    for (u16 i = 1; i < total; i++) {
        void *p = kmalloc(32);
        if (!p) break;
        got++;
    }
    ASSERT_EQ(got, total);
}

/* ---- kcaches list state machine ----------------------------------------- */

/* filling a slab must move it partial_slabs -> full_slabs */
TEST(slab, filled_slab_moves_to_full_list) {
    slab_boot();
    void *first = kmalloc(2048);
    ASSERT_NE(first, 0);
    slab *s = slab_of(first);
    u16 total = s->total;
    for (u16 i = 1; i < total; i++) ASSERT_NE(kmalloc(2048), 0);

    if (s->inuse != total)
        test_failf(__FILE__, __LINE__,
            "slab not full after %u allocs (inuse=%u)", total, s->inuse);
    if (!on_any_full(s) || on_any_partial(s))
        test_failf(__FILE__, __LINE__,
            "filled slab not moved to full_slabs (on_full=%d on_partial=%d) -- "
            "full_slabs is never populated", on_any_full(s), on_any_partial(s));
}

/* freeing an object from a full slab must leave it reachable again for reuse */
TEST(slab, freed_slot_in_full_slab_is_reusable) {
    slab_boot();
    void *first = kmalloc(2048);
    ASSERT_NE(first, 0);
    slab *s = slab_of(first);
    u16 total = s->total;
    void *last = first;
    for (u16 i = 1; i < total; i++) { last = kmalloc(2048); ASSERT_NE(last, 0); }

    kfree(last);                          /* opens one slot in the (full) slab */
    void *reuse = kmalloc(2048);
    if (!reuse)
        test_failf(__FILE__, __LINE__,
            "kmalloc after freeing from a full slab returned NULL -- the slab was "
            "not returned to partial_slabs");
    if (slab_of(reuse) != s)
        test_failf(__FILE__, __LINE__, "reuse did not come from the freed slab");
}

/* reclaiming a slab (freeing its last object) must UNLINK it from the cache --
 * directly pins the reclaim UAF (buddy_free without list_remove) */
TEST(slab, reclaimed_slab_is_unlinked) {
    slab_boot();
    void *o = kmalloc(32);
    ASSERT_NE(o, 0);
    slab *s = slab_of(o);
    ASSERT_TRUE(on_any_partial(s));       /* sanity: linked while live */

    kfree(o);                             /* last object -> reclaimed */
    if (on_any_partial(s) || on_any_full(s))
        test_failf(__FILE__, __LINE__,
            "reclaimed slab 0x%llx still linked (partial=%d full=%d) -- _slab_free "
            "buddy_frees without list_remove (dangling entry / UAF)",
            ULL((uintptr_t)s), on_any_partial(s), on_any_full(s));
}

/* the cache lists stay well-formed: no cycles, no slab on both lists, and every
 * slab on partial_slabs is still a live SLAB-owned page (not freed-but-linked) */
TEST(slab, cache_lists_stay_consistent) {
    slab_boot();
    void *a = kmalloc(64), *b = kmalloc(64), *c = kmalloc(256);
    ASSERT_NE(a, 0); ASSERT_NE(b, 0); ASSERT_NE(c, 0);
    kfree(b);                             /* slab still has `a` -> not reclaimed */

    for (int i = 0; i < CACHES; i++) {
        list_head_t *pl = &kcaches[i].partial_slabs;
        int guard = 0;
        for (list_head_t *n = pl->next; n != pl; n = n->next) {
            if (++guard > 4096)
                test_failf(__FILE__, __LINE__, "cycle in partial_slabs[%d]", i);
            slab *s = list_entry(n, slab, list);
            if (on_any_full(s))
                test_failf(__FILE__, __LINE__,
                    "slab 0x%llx is on both partial and full lists", ULL((uintptr_t)s));
            void *pg = (void *)(uintptr_t)ALIGN_DOWN((uintptr_t)s, PAGE_SIZE);
            if (get_page_owner(pg) != PAGE_SLAB)
                test_failf(__FILE__, __LINE__,
                    "partial_slabs[%d] holds slab 0x%llx whose page is not SLAB-owned "
                    "(owner=%d) -- freed but still linked", i, ULL((uintptr_t)s),
                    get_page_owner(pg));
        }
    }
}
