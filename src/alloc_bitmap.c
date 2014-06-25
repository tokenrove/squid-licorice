
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
    if (me->j == LIMB_SIZE) {
        ++me->i; me->j = 0;
    }
    ENSURE(me->i < t->count && me->j < LIMB_SIZE);
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
    remove_member(t, me->i, me->j);
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

static void create_iterate_remove(size_t n)
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
        ok(NULL != p);
        cmp_ok(*p, "==", i, "iteration should be incremental");
        if (*p % 2) it.mark_for_removal(&it);
    }
    it.expunge_marked(&it);

    it = alloc_bitmap_iterate(bm);
    for (size_t i = 0; i < n; i += 2) {
        size_t *p = it.next(&it);
        ok(NULL != p);
        cmp_ok(*p, "==", i, "we should have removed every second one");
    }

    alloc_bitmap_destroy(bm);
}

int main(void)
{
    plan(1);
    create_iterate_remove(1000);
    done_testing();
}
#endif
