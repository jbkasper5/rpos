/*
 * Page-frame-array initialization invariants.
 *
 * These encode "what must be true immediately after
 * initialize_page_frame_array()" -- the reserved boundary and the fact that
 * the metadata array's own pages are reserved. This is exactly the property
 * that was violated before the get_phys_test_region()/get_test_size() fix.
 */
#include "test.h"
#include "host_env.h"
#include "memory/paging.h"

static page_frame_t *PFA;
static u64           RESV;

static void boot(void) {
    u64 returned = initialize_page_frame_array();
    PFA  = host_pfa();
    RESV = host_reserved_pages();
    /* the function reports the reserved-prefix length it computed */
    ASSERT_EQ(returned, RESV);
}

TEST(pfa, init_returns_reserved_count) {
    boot();
}

TEST(pfa, reserved_prefix_all_reserved) {
    boot();
    for (u64 p = 0; p < RESV; p++)
        ASSERT_EQ(PFA[p].flags.bits.state, PAGE_RESERVED);
}

TEST(pfa, buddy_region_boundary) {
    boot();
    /* last reserved frame is reserved; first buddy frame is not */
    ASSERT_EQ(PFA[RESV - 1].flags.bits.state, PAGE_RESERVED);
    ASSERT_NE(PFA[RESV].flags.bits.state, PAGE_RESERVED);
}

TEST(pfa, metadata_array_is_itself_reserved) {
    boot();
    /* the frame_metadata array's own physical pages must fall in the prefix */
    u64 phys  = va_to_pa(page_frame_array_start());
    u64 first = phys >> 12;
    u64 last  = (phys + HOST_NPAGES * sizeof(page_frame_t) - 1) >> 12;
    ASSERT_LT(last, RESV);                                   /* fully inside prefix */
    ASSERT_EQ(PFA[first].flags.bits.state, PAGE_RESERVED);
    ASSERT_EQ(PFA[last].flags.bits.state,  PAGE_RESERVED);
}
