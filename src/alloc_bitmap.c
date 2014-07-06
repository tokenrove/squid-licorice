
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "alloc_bitmap.h"
#include "ensure.h"
#include "log.h"

enum { SHIFT = 5, MASK = 0x1f, LIMB_SIZE = 32 };
typedef uint32_t limb;

struct t {
    size_t count, member_size, actual;
    limb *bits;
    void *members;
};

/* Per Hacker's Delight, 3-2 */
static inline limb closest_power_of_2(limb x)
{
    return 1 << (LIMB_SIZE - __builtin_clz(x-1));
}

alloc_bitmap alloc_bitmap_init(size_t count, size_t member_size)
{
    size_t orig_count = count;
    count = closest_power_of_2(count);
    if (orig_count != count)
        LOG_DEBUG("count wasn't a power of 2 (%d), rounding to %d", orig_count, count);
    if (count < 1+MASK) {
        LOG_DEBUG("count was less than the minimum size (%d), rounding to %d", count, 1+MASK);
        count = 1+MASK;
    }
    if (member_size & (sizeof (void*)-1))
        LOG_DEBUG("member size probably isn't properly aligned: %d", member_size);
    ENSURE(member_size);

    struct t *t;
    ENSURE(t = calloc(1, sizeof (*t)));
    ENSURE(t->members = calloc(count, member_size));
    count >>= SHIFT;
    ENSURE(t->bits = calloc(count, sizeof (limb)));
    t->count = count;
    t->member_size = member_size;
    return t;
}

void alloc_bitmap_destroy(alloc_bitmap t_)
{
    struct t *t = t_;
    free(t->members);
    free(t->bits);
    memset(t, 0, sizeof (*t));
    free(t);
}

static inline void *address_of_member(struct t *t, size_t word, size_t bit)
{
    return (void *)&((uint8_t*)t->members)[t->member_size * (bit + (word<<SHIFT))];
}

static inline size_t member_at_address(struct t *t, intptr_t m)
{
    intptr_t p = (intptr_t)t->members;
    return (m-p)/t->member_size;
}

void *alloc_bitmap_alloc_first_free(alloc_bitmap t_)
{
    struct t *t = t_;
    for (size_t word = 0; word < t->count; ++word) {
        if (0xffffffff == t->bits[word]) continue;
        size_t bit = ffs(~t->bits[word]);
        ENSURE(bit);
        t->bits[word] |= 1<<(bit-1);
        ++t->actual;
        return address_of_member(t, word, bit-1);
    }
    return NULL;
}

static void remove_member(struct t *t, size_t word, size_t bit)
{
    memset(address_of_member(t, word, bit),
           0,
           t->member_size);
    t->bits[word] &= ~(1<<bit);
    ENSURE(t->actual-- > 0);
}

bool alloc_bitmap_remove(alloc_bitmap t_, void *m)
{
    struct t *t = t_;
    size_t i = member_at_address(t, (intptr_t)m),
        word = i>>SHIFT,
         bit = i&MASK;
    bool present = t->bits[word] & (1<<bit);
    remove_member(t, word, bit);
    return present;
}


static void *it_next(struct alloc_bitmap_iterator *me)
{
    struct t *t = me->t;
    if (me->j == LIMB_SIZE) { ++me->i; me->j = 0; }
    for (; me->i < t->count && me->n < t->actual; ++me->i, me->j = 0) {
        limb x = t->bits[me->i] >> me->j;
        if (0 == x) continue;
        size_t bit = ffs(x);
        ENSURE(bit);
        me->j += bit;
        ++me->n;
        return address_of_member(t, me->i, me->j-1);
    }
    return NULL;
}

static void it_mark_for_removal(struct alloc_bitmap_iterator *me)
{
    struct t *t = me->t;
    --me->n;
    remove_member(t, me->i, me->j-1);
}

static void it_expunge_marked(struct alloc_bitmap_iterator *_ __attribute__((__unused__))) {}

struct alloc_bitmap_iterator alloc_bitmap_iterate(alloc_bitmap bitmap)
{
    return (struct alloc_bitmap_iterator){
        .t = bitmap,
        .i = 0, .j = 0, .n = 0,
        .next = it_next,
        .mark_for_removal = it_mark_for_removal,
        .expunge_marked = it_expunge_marked
    };
}

#ifdef UNIT_TEST_ALLOC_BITMAP
#include "libtap/tap.h"

static void test_create_iterate_remove(size_t n)
{
    note("%s(%d)", __func__, n);
    alloc_bitmap bm = alloc_bitmap_init(n, sizeof (size_t));
    for (size_t i = 0; i < n; ++i) {
        size_t *p = alloc_bitmap_alloc_first_free(bm);
        *p = i;
    }
    struct alloc_bitmap_iterator it = alloc_bitmap_iterate(bm);
    for (size_t i = 0; i < n; ++i) {
        size_t *p = it.next(&it);
        ENSURE(p && *p == i);
        if (*p % 2) it.mark_for_removal(&it);
    }
    ENSURE(NULL == it.next(&it));
    it.expunge_marked(&it);

    it = alloc_bitmap_iterate(bm);
    for (size_t i = 0; i < n; i += 2) {
        size_t *p = it.next(&it);
        ENSURE(p && *p == i);
    }
    ENSURE(NULL == it.next(&it));

    alloc_bitmap_destroy(bm);
}

static void test_overflow(void)
{
    note("Test overflow");
    const int n = closest_power_of_2(12*1024);
    alloc_bitmap bm = alloc_bitmap_init(n, 1);
    for (int i = 0; i < n; ++i) {
        uint8_t *p = alloc_bitmap_alloc_first_free(bm);
        ENSURE(p && *p == 0);
    }
    ok(NULL == alloc_bitmap_alloc_first_free(bm), "No room at the inn.");
    for (int i = 0; i < 42; ++i)
        ENSURE(NULL == alloc_bitmap_alloc_first_free(bm));
    alloc_bitmap_destroy(bm);
}

static void test_tiny_bitmap(void)
{
    alloc_bitmap bm = alloc_bitmap_init(1, sizeof(intptr_t));
    intptr_t *p = alloc_bitmap_alloc_first_free(bm);
    *p = 42;
    alloc_bitmap_remove(bm, p);
    alloc_bitmap_destroy(bm);
}

static void test_3270f2291199b735e46d6d00d1e905d1531e7f21(void)
{
    note("Regression test for bug revealed by commit 3270f2291199b735e46d6d00d1e905d1531e7f21");
    int n = LIMB_SIZE;
    alloc_bitmap bm = alloc_bitmap_init(n, sizeof(intptr_t));
    while (alloc_bitmap_alloc_first_free(bm));
    ok(NULL == alloc_bitmap_alloc_first_free(bm), "Definitely full.");
    struct alloc_bitmap_iterator iter = alloc_bitmap_iterate(bm);
    int i = 0;
    while (iter.next(&iter)) ++i;
    cmp_ok(i, "==", n, "Got as many items as we expected");
    ok(NULL == iter.next(&iter), "Iterator is definitely done.");
    alloc_bitmap_destroy(bm);
}

int main(void)
{
    plan(9);
    lives_ok({test_create_iterate_remove(1000);}, "regular bitmap");
    // TODO: should improve the speed of these routines so a 1M bitmap can be tested
    lives_ok({test_create_iterate_remove(24*1024);}, "larger bitmap");
    dies_ok({alloc_bitmap_init(1000, 0);}, "member size can't be 0");
    lives_ok({test_tiny_bitmap();}, "tiny bitmap");
    lives_ok({test_overflow();}, "Test overflow");
    test_3270f2291199b735e46d6d00d1e905d1531e7f21();
    done_testing();
}
#endif


#ifdef PROFILE_ALLOC_BITMAP
int main(void)
{
    size_t n = 1024*1024;

    alloc_bitmap bm = alloc_bitmap_init(n, sizeof (size_t));
    for (size_t i = 0; i < n; ++i) {
        size_t *p = alloc_bitmap_alloc_first_free(bm);
        *p = i;
    }
    struct alloc_bitmap_iterator it = alloc_bitmap_iterate(bm);
    for (size_t i = 0; i < n; ++i) {
        size_t *p = it.next(&it);
        ENSURE(p && *p == i);
    }
    ENSURE(NULL == it.next(&it));
    alloc_bitmap_destroy(bm);
}
#endif
