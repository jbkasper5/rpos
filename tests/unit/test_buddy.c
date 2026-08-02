/*
 * Buddy allocator invariants.
 *
 * Each assertion is a property that the on-target profiler surfaced by hand
 * this session: allocated heads/tails carry the right state+flags, byte sizes
 * round to the right order, allocations never overlap or dip into the reserved
 * prefix, and the tail-page metadata (the recently-fixed state=BUDDY on tails)
 * is correct.
 */
#include "test.h"
#include "host_env.h"
#include "memory/paging.h"

static page_frame_t *PFA;
static u64           RESV;

static void boot(void) {
    initialize_page_frame_array();
    PFA  = host_pfa();
    RESV = host_reserved_pages();
}

/* allocate `bytes`, assert success, return the head PFN */
static u64 alloc_pfn(u64 bytes) {
    u64 va = buddy_alloc(bytes);
    ASSERT_NE(va, 0);
    return va_to_pa(va) >> 12;
}

TEST(buddy, single_page_head_metadata) {
    boot();
    u64 pfn = alloc_pfn(PAGE_SIZE);
    ASSERT_GE(pfn, RESV);                                   /* from buddy region */
    ASSERT_EQ(PFA[pfn].flags.bits.state, PAGE_BUDDY);
    ASSERT_TRUE(PFA[pfn].flags.bits.flags & PAGE_BUDDY_HEAD);
    ASSERT_EQ(PFA[pfn].order, 0);
}

TEST(buddy, two_page_block_head_and_tail) {
    boot();
    u64 pfn = alloc_pfn(2 * PAGE_SIZE);

    /* head */
    ASSERT_EQ(PFA[pfn].flags.bits.state, PAGE_BUDDY);
    ASSERT_TRUE(PFA[pfn].flags.bits.flags & PAGE_BUDDY_HEAD);
    ASSERT_EQ(PFA[pfn].order, 1);

    /* tail: allocated (BUDDY), flagged TAIL, .order overloaded to head PFN */
    ASSERT_EQ(PFA[pfn + 1].flags.bits.state, PAGE_BUDDY);   /* the tail-state fix */
    ASSERT_TRUE(PFA[pfn + 1].flags.bits.flags & PAGE_BUDDY_TAIL);
    ASSERT_EQ(PFA[pfn + 1].order, pfn);
}

TEST(buddy, byte_size_rounds_up_to_order) {
    boot();
    u64 pfn = alloc_pfn(PAGE_SIZE + 1);                     /* 4097 B -> 2 pages -> order 1 */
    ASSERT_EQ(PFA[pfn].order, 1);
}

TEST(buddy, allocations_never_overlap) {
    boot();
    u64 sizes[] = { PAGE_SIZE, 2 * PAGE_SIZE, PAGE_SIZE,
                    4 * PAGE_SIZE, PAGE_SIZE, 8 * PAGE_SIZE };
    int n = (int)(sizeof(sizes) / sizeof(sizes[0]));
    u64 lo[16], hi[16];

    for (int i = 0; i < n; i++) {
        u64 pfn   = alloc_pfn(sizes[i]);
        u64 pages = sizes[i] >> 12;
        lo[i] = pfn;
        hi[i] = pfn + pages;
        ASSERT_GE(pfn, RESV);                               /* never in the reserved prefix */
        for (int j = 0; j < i; j++)
            ASSERT_TRUE(hi[i] <= lo[j] || hi[j] <= lo[i]);  /* disjoint ranges */
    }
}

TEST(buddy, order0_alloc_free_roundtrip) {
    boot();
    u64 pfn = alloc_pfn(PAGE_SIZE);
    ASSERT_EQ(PFA[pfn].flags.bits.state, PAGE_BUDDY);
    buddy_free((void *)(uintptr_t)pa_to_va(pfn << 12));
    ASSERT_EQ(PFA[pfn].flags.bits.state, PAGE_FREE);
}

TEST(buddy, get_block_order_reports_size) {
    boot();
    u64 pfn = alloc_pfn(4 * PAGE_SIZE);                     /* order 2 */
    ASSERT_EQ(get_block_order(pfn << 12), 2);
}

TEST(buddy, set_get_page_owner_roundtrip) {
    boot();
    u64   pfn = alloc_pfn(PAGE_SIZE);
    void *va  = (void *)(uintptr_t)pa_to_va(pfn << 12);
    set_page_owner(va, PAGE_SLAB);
    ASSERT_EQ(get_page_owner(va), PAGE_SLAB);
}

TEST(buddy, head_from_tail_resolves_to_head) {
    boot();
    u64   pfn     = alloc_pfn(2 * PAGE_SIZE);
    void *tail_va = (void *)(uintptr_t)pa_to_va((pfn + 1) << 12);
    void *head    = head_from_page(tail_va);
    ASSERT_EQ(va_to_pa((u64)(uintptr_t)head) >> 12, pfn);
}
