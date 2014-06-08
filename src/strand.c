/* strand: green threads */

#include <ucontext.h>

#include "ensure.h"
#include "strand.h"

#ifndef NVALGRIND
#include "valgrind.h"
#else
#define VALGRIND_STACK_REGISTER(...)
#endif

struct t {
    ucontext_t context;
    ucontext_t parent;
    float dt;
    bool is_alive;
};

static void strand_wrap_0(strand self, int (*fn)(strand))
{
    (*fn)(self);
    struct t *st = self;
    st->is_alive = false;
}

static void strand_wrap_1(strand self, int (*fn)(strand, void *), void *arg)
{
    (*fn)(self, arg);
    struct t *st = self;
    st->is_alive = false;
}

static strand spawn(size_t size_in_words)
{
    struct t *st = calloc(1, sizeof (*st));
    st->is_alive = true;
    getcontext(&st->parent);
    getcontext(&st->context);
    st->context.uc_link = &st->parent;
    size_t size = size_in_words * sizeof (void *);
    posix_memalign(&st->context.uc_stack.ss_sp, 16, size);
    (void)VALGRIND_STACK_REGISTER(st->context.uc_stack.ss_sp, st->context.uc_stack.ss_sp + size);
    st->context.uc_stack.ss_size = size;
    return st;
}

strand strand_spawn_0(void (*fn)(strand), size_t size)
{
    struct t *st = spawn(size);
    makecontext(&st->context, (void(*)(void))strand_wrap_0, 2, st, fn);
    return st;
}

strand strand_spawn_1(void (*fn)(strand, void *), size_t size, void *arg)
{
    struct t *st = spawn(size);
    makecontext(&st->context, (void(*)(void))strand_wrap_1, 3, st, fn, arg);
    return st;
}

void strand_resume(strand strand_, float dt)
{
    struct t *st = strand_;
    st->dt = dt;
    swapcontext(&st->parent, &st->context);
}

float strand_yield(strand self)
{
    struct t *st = self;
    swapcontext(&st->context, &st->parent);
    return st->dt;
}

bool strand_is_alive(strand st)
{
    return ((struct t *)st)->is_alive;
}

void strand_destroy(strand strand_)
{
    struct t *st = strand_;
    free(st->context.uc_stack.ss_sp);
    memset(st, 0, sizeof (*st));
    free(st);
}
