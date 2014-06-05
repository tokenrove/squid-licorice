/* strand: green threads */

#include <ucontext.h>

#include "ensure.h"
#include "strand.h"

struct strand {
    ucontext_t context;
    ucontext_t parent;
    float dt;
    bool is_alive;
};

static void strand_wrap(strand self, int (*fn)(strand))
{
    (*fn)(self);
    struct strand *st = self;
    st->is_alive = false;
}

strand strand_spawn(void (*fn)(strand), size_t size)
{
    struct strand *st = calloc(1, sizeof (*st));
    st->is_alive = true;
    getcontext(&st->parent);
    getcontext(&st->context);
    st->context.uc_link = &st->parent;
    posix_memalign(&st->context.uc_stack.ss_sp, 16, size * sizeof (void*));
    st->context.uc_stack.ss_size = size;
    makecontext(&st->context, (void(*)(void))strand_wrap, 2, st, fn);
    return st;
}

void strand_resume(strand strand_, float dt)
{
    struct strand *st = strand_;
    st->dt = dt;
    swapcontext(&st->parent, &st->context);
}

float strand_yield(strand self)
{
    struct strand *st = self;
    swapcontext(&st->context, &st->parent);
    return st->dt;
}

bool strand_is_alive(strand st)
{
    return ((struct strand *)st)->is_alive;
}
