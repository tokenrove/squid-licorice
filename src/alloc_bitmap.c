
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "alloc_bitmap.h"
#include "ensure.h"

enum { SHIFT = 5, MASK = 0x1f };

struct t {
    size_t count, member_size, actual;
    uint32_t *bits;
    void *members;
};

/* Bad things will probably happen if count isn't a power of two, and
 * if member_size isn't reasonably aligned for your structure of
 * choice.  Don't do that.
 */
alloc_bitmap alloc_bitmap_init(size_t count, size_t member_size)
{
    struct t *t;
    ENSURE(t = calloc(1, sizeof (*t)));
    ENSURE(t->members = calloc(count, member_size));
    count >>= SHIFT;
    ENSURE(t->bits = calloc(count, sizeof (uint32_t)));
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
    ENSURE(me->i < t->count && me->j < 32);
    for (; me->i < t->count && me->n < t->actual; ++me->i, me->j = 0) {
        uint32_t x = t->bits[me->i] >> me->j;
        if (0 == x) continue;
        size_t bit = ffs(x);
        ENSURE(bit);
        me->j += bit-1;
        ++me->n;
        return address_of_member(t, me->i, me->j);
    }
    return NULL;
}

static void it_mark_for_removal(struct alloc_bitmap_iterator *me)
{
    struct t *t = me->t;
    remove_member(t, me->i, me->j);
}

static void it_expunge_marked(struct alloc_bitmap_iterator *unused) {}

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
