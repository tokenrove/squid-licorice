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
    void fn(strand self, void *data) {
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

static void test_recursive_1(void)
{
    void fn(strand self) {
        static int n = 10;
        while (--n > 0) {
            strand_yield(self);
            fn(self);
        }
    };
    strand s = strand_spawn_0(fn, STRAND_DEFAULT_STACK_SIZE);
    ok(NULL != s);
    int count = 1;
    while (strand_is_alive(s)) {
        ++count;
        strand_resume(s, 1.);
    }
    cmp_ok(count, "==", 10);
}

static long simple_factorial(strand self, int n)
{
    if (n <= 0) return 1;
    strand_yield(self);
    return n * simple_factorial(self, n-1);
}

static void test_recursive_2(void)
{
    long output;
    void fn(strand self, void *data) {
        output = simple_factorial(self, (intptr_t)data);
    };
    strand s = strand_spawn_1(fn, STRAND_DEFAULT_STACK_SIZE, (void *)12);
    ok(NULL != s);
    int count = 0;
    while (strand_is_alive(s)) {
        ++count;
        strand_resume(s, 1.);
    }
    cmp_ok(count, "==", 12);
    cmp_ok(output, "==", 479001600);
}

static void test_basic_usage(void)
{
    note("Basic usage");
    lives_ok({run_two_strands_to_completion();}, "Run two strands to completion.");

    lives_ok({
            bool has_run = false;
            void fn(strand _ __attribute__ ((unused))) {
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

    lives_ok({test_recursive_1();}, "Test a basic recursive strand.");
    lives_ok({test_recursive_2();});
}

static void test_too_small_stack(void)
{
    note("Test stack overflow");

    void fn_a(strand _ __attribute__ ((unused))) {
        int space[20];
        space[19] = 42;
        fprintf(stderr, "space[19] == %d\n", space[19]);
    };
    dies_ok({
            strand a = strand_spawn_0(fn_a, 10);
            ENSURE(a);
        }, "Not enough space to begin with");
    void fn_b(strand self) {
        strand_yield(self);
        fn_b(self);
    };
    dies_ok({
            strand s = strand_spawn_0(fn_b, STRAND_DEFAULT_STACK_SIZE);
            while (strand_is_alive(s))
                strand_resume(s, 1.);
            strand_destroy(s);
        }, "Recursive function exhausts stack space");

    note("TODO: Is there a way to easily test stack underflow?");
}

static void test_nested_threads_1(int initial_count, int n_children)
{
    void fn_a(strand self, void *data_) {
        int *data = data_;
        while (true) {
            float dt = strand_yield(self);
            *data -= (int)dt;
        }
    }
    int counter;
    void fn_b(strand self) {
        strand children[n_children];
        for (int i = 0; i < n_children; ++i) {
            children[i] = strand_spawn_1(fn_a, STRAND_DEFAULT_STACK_SIZE, &counter);
            ok(NULL != children[i]);
        }
        float dt = strand_yield(self);
        while (counter > 0) {
            for (int i = 0; i < n_children; ++i)
                strand_resume(children[i], dt);
            dt = strand_yield(self);
        }
        for (int i = 0; i < n_children; ++i)
            strand_destroy(children[i]);
    }

    note("test_nested_threads_1(%d, %d)", initial_count, n_children);
    counter = initial_count;
    strand s = strand_spawn_0(fn_b, STRAND_DEFAULT_STACK_SIZE);
    int n = 0;
    while (strand_is_alive(s)) {
        strand_resume(s, 1.);
        ++n;
    }
    strand_destroy(s);
    cmp_ok(counter, "==", -n_children + (initial_count%n_children));
    cmp_ok(n, "==", 2+(initial_count/n_children));
}

static void test_nested_threads_2(int depth)
{
    void fn(strand self, void *data) {
        intptr_t n = (intptr_t)data;
        if (n < 0) return;
        strand_yield(self);
        strand child = strand_spawn_1(fn, STRAND_DEFAULT_STACK_SIZE, (void *)(n-1));
        while (strand_is_alive(child)) {
            float dt = strand_yield(self);
            strand_resume(child, dt);
        }
        strand_destroy(child);
    }
    intptr_t n = depth;

    note("test_nested_threads_2(%d)", depth);
    strand s = strand_spawn_1(fn, STRAND_DEFAULT_STACK_SIZE, (void *)n);
    while (strand_is_alive(s)) strand_resume(s, 1.);
    strand_destroy(s);
}

static void test_many_threads_1(int n)
{
    void fn(strand self) {
        static double t = 10.;
        while (t > 0.)
            t -= strand_yield(self);
    };
    strand s[n];

    note("test_many_threads_1(%d)", n);
    for (int i = 0; i < n; ++i)
        s[i] = strand_spawn_0(fn, 32);

    bool any_alive;
    do {
        any_alive = false;
        for (int i = 0; i < n; ++i) {
            strand_resume(s[i], 1.);
            any_alive |= strand_is_alive(s[i]);
        }
    } while(any_alive);

    for (int i = 0; i < n; ++i)
        strand_destroy(s[i]);
}

int main(void)
{
    long seed = time(NULL);
    note("srand48(%ld)\n", seed);
    srand48(seed);
    plan(40);
    test_basic_usage();
    test_too_small_stack();
    lives_ok({test_nested_threads_1(42, 4);});
    lives_ok({test_nested_threads_1(107, 12);});
    lives_ok({test_nested_threads_2(120);});
    lives_ok({test_many_threads_1(12*1024);});
    done_testing();
}
#endif
