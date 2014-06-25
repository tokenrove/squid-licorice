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
    swapcontext(&st->parent, &st->context);  /* schedule strand once */
    return st;
}

strand strand_spawn_1(void (*fn)(strand, void *), size_t size, void *arg)
{
    struct t *st = spawn(size);
    makecontext(&st->context, (void(*)(void))strand_wrap_1, 3, st, fn, arg);
    swapcontext(&st->parent, &st->context);  /* schedule strand once */
    return st;
}

void strand_resume(strand strand_, float dt)
{
#ifdef DEBUG
    /* We disallow dt less than some epsilon to avoid numerical
     * instability, at least in a debug build. */
    ENSURE(dt > 0.001f);
#endif
    struct t *st = strand_;
    if (!st->is_alive) return;
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

#ifdef UNIT_TEST_STRAND
#include "libtap/tap.h"

static void run_two_strands_to_completion(void)
{
    unsigned count_a = 0, count_b = 0;
    void fn(void *self, void *data) {
        double accum = 0.;
        while (accum < 1.) {
            ++*(unsigned *)data;
            accum += strand_yield(self);
        }
    };
    strand a = strand_spawn_1(fn, 32, &count_a);
    strand b = strand_spawn_1(fn, 32, &count_b);
    while (strand_is_alive(a) || strand_is_alive(b)) {
        if (strand_is_alive(a)) strand_resume(a, drand48());
        if (strand_is_alive(b)) strand_resume(b, drand48());
    }
    strand_destroy(b);
    strand_destroy(a);
    cmp_ok(count_a, ">", 1);
    cmp_ok(count_b, ">", 1);
}

static void test_basic_usage(void)
{
    lives_ok({run_two_strands_to_completion();}, "Run two strands to completion.");

    lives_ok({
            bool has_run = false;
            void fn(void *_ __attribute__ ((unused))) {
                has_run = true;
                /* exit immediately */
            };
            strand s = strand_spawn_0(fn, STRAND_DEFAULT_STACK_SIZE);
            ok(NULL != s);
            ok(!strand_is_alive(s));
            ok(has_run, "Strand completed work when spawned");
            strand_resume(s, 1.);
            strand_destroy(s);
        }, "Run a strand that exits immediately.");
}

static void test_too_small_stack(void)
{
    void fn_a(void *self) {
        int space[20];
        space[19] = 42;
        fprintf(stderr, "space[19] == %d\n", space[19]);
    };
    dies_ok({
            strand a = strand_spawn_0(fn_a, 10);
        }, "Not enough space to begin with");
    void fn_b(void *self) {
        strand_yield(self);
        fn_b(self);
    };
    dies_ok({
            strand s = strand_spawn_0(fn_b, STRAND_DEFAULT_STACK_SIZE);
            while (strand_is_alive(s))
                strand_resume(s, 1.);
            strand_destroy(s);
        }, "Recursive function exhausts stack space");
}

int main(void)
{
    long seed = time(NULL);
    note("srand48(%ld)\n", seed);
    srand48(seed);
    plan(9);
    test_basic_usage();
    test_too_small_stack();
    done_testing();
}
#endif
