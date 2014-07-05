
#include "physics.h"
#include "alloc_bitmap.h"
#include "ensure.h"
#include "video.h"
#include "inline_math.h"

static alloc_bitmap bodies;

void bodies_init(size_t n)
{
    ENSURE(bodies = alloc_bitmap_init(n, sizeof (struct body)));
}

void bodies_destroy(void)
{
    alloc_bitmap_destroy(bodies);
    bodies = NULL;
}

struct body *body_new(position p, float collision_radius, float mass)
{
    struct body *b = (struct body *)alloc_bitmap_alloc_first_free(bodies);
    *b = (struct body){.p = p,
                       .collision_radius = collision_radius,
                       .mass = mass};
    return b;
}

void body_destroy(struct body *body)
{
    alloc_bitmap_remove(bodies, body);
    // XXX remove references to this in the batch?  write a test
}

static void body_update(struct body *body, float dt)
{
    const float friction = 0.08f;
    position accel = body->F / body->mass;
    position delta = body->impulses / body->mass + accel*dt;
    body->impulses = 0;
    /* per http://www.niksula.hut.fi/~hkankaan/Homepages/gravity.html */
    body->v += delta / 2;
    body->p += body->v;
    body->v += delta / 2;
    body->v -= friction * body->v;  /* Stokes' drag */
}

/* TODO:
 * - more shapes;
 * - restitution information
 */
static void check_collisions_against(struct body *a, float dt)
{
    struct alloc_bitmap_iterator iter = alloc_bitmap_iterate(bodies);
    struct body *b;
    while((b = iter.next(&iter))) {
        if (a == b || NULL == a->collision_fn) continue;
        double d = cabsf(a->p - b->p);
        double r = maxf(a->collision_radius, b->collision_radius);
        if (d*d < r*r)
            (*a->collision_fn)(a, b, a->data);
    }
}

static void apply_offside_rule(struct body *b)
{
    if (crealf(b->p) < -b->collision_radius/2 ||
        crealf(b->p) > viewport_w + b->collision_radius/2 ||
        cimagf(b->p) < -b->collision_radius/2 ||
        cimagf(b->p) > viewport_h + b->collision_radius/2)
        if (b->collision_fn)
            (*b->collision_fn)(b, NULL, b->data);
}

static void check_collisions(float dt)
{
    struct alloc_bitmap_iterator iter = alloc_bitmap_iterate(bodies);
    struct body *b;
    while((b = iter.next(&iter))) {
        apply_offside_rule(b);
        check_collisions_against(b, dt);
    }
}

void bodies_update(float dt)
{
    struct alloc_bitmap_iterator iter = alloc_bitmap_iterate(bodies);
    struct body *b;
    while((b = iter.next(&iter)))
        body_update(b, dt);

    check_collisions(dt);
}

#ifdef UNIT_TEST_PHYSICS
#include "libtap/tap.h"

uint16_t viewport_w = 640, viewport_h = 480;

static position random_position(void)
{
    return drand48() + I*drand48();
}

#define DUMP_BODY(v) (_dump_body(v, #v))

static void _dump_body(struct body *a, const char *name)
{
    note("%s: {p:(%f,%f), v:(%f,%f)}, cr: %f", name, crealf(a->p), cimagf(a->p),
         crealf(a->v), cimagf(a->v), a->collision_radius);
}

static void simple_test(unsigned n_bodies, unsigned n_iterations)
{
    bodies_init(n_bodies);
    for (unsigned i = 0; i < 2*n_bodies; ++i) {
        struct body *b;
        b = body_new(random_position(), drand48()/n_bodies, drand48());
        if (i%2) body_destroy(b);
    }
    for (unsigned i = 0; i < n_iterations; ++i)
        bodies_update(drand48());
    bodies_destroy();
}

static void test_simple_collision_occurs(void)
{
    struct body *a, *b;
    void fn(struct body *us, struct body *them, void *data) {
        ok(us == a);
        ok(them == b);
        *(bool *)data = true;
    }
    bodies_init(2);
    bool a_collided = false;
    a = body_new(0., 10., 1.);
    ok(NULL != a);
    a->data = &a_collided;
    a->collision_fn = fn;
    b = body_new(15., 10., 1.);
    ok(NULL != b);
    bodies_update(1.);
    cmp_ok(a_collided, "==", false);
    for (int i = 5; i > 0; --i) {
        b->impulses = -1.;
        bodies_update(1.);
        if (a_collided) goto end;
    }
    fail("Bodies didn't collide when they should have.");
    DUMP_BODY(a);
    DUMP_BODY(b);
end:
    bodies_destroy();
}

static void test_specific_collision_regression_1(void)
{
    bool was_called = false;
    struct body *a, *b;
    void fn(struct body *us, struct body *them, void *data) {
        fail("collision handler incorrectly called; parameters %p, %p, %p", us, them, data);
        *(bool *)data = true;
    }
    bodies_init(2);
    a = body_new(0.135395 + I*0.587962, 0.000003, 1.);
    a->collision_fn = fn;
    a->data = &was_called;
    b = body_new(0.098993 + I*0.551578, 0.000005, 1.);
    b->collision_fn = fn;
    b->data = &was_called;
    bodies_update(1.);
    ok(!was_called);
    bodies_destroy();
}

static void test_specific_collision_regression_2(void)
{
    bool was_called_ab = false, was_called_ba = false;
    struct body *a, *b;
    void fn(struct body *us, struct body *them, void *data) {
        bool *p = data;

        if (us == a) {
            ok(them == b);
            ok(p == &was_called_ab);
        } else if (us == b) {
            ok(them == a);
            ok(p == &was_called_ba);
        } else
            fail("collision handler incorrectly called with parameters %p, %p, %p", us, them, data);
        *p = true;
    }
    bodies_init(2);
    a = body_new(0.135395 + I*0.587962, 0.1, 1.);
    a->collision_fn = fn;
    a->data = &was_called_ab;
    b = body_new(0.098993 + I*0.551578, 0.1, 1.);
    b->collision_fn = fn;
    b->data = &was_called_ba;
    bodies_update(1.);
    ok(was_called_ab);
    ok(was_called_ba);
    bodies_destroy();
}

int main(void)
{
    plan(13);
    long seed = time(NULL);
    note("srand48(%ld)\n", seed);
    srand48(seed);
    test_specific_collision_regression_1();
    test_specific_collision_regression_2();
    test_simple_collision_occurs();
    lives_ok({simple_test(1000, 100);});
    done_testing();
}
#endif
