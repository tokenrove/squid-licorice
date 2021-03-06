#include "physics.h"
#include "alloc_bitmap.h"
#include "ensure.h"
#include "video.h"
#include "inline_math.h"
#include "msg.h"
#include "msg_macros.h"

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

struct body *body_new(position p, float collision_radius)
{
    struct body *b = (struct body *)alloc_bitmap_alloc_first_free(bodies);
    *b = (struct body){.p = p,
                       .collision_radius = collision_radius,
                       .mass = 1.};
    return b;
}

void body_destroy(struct body *body)
{
    alloc_bitmap_remove(bodies, body);
    // XXX remove references to this in the batch?  write a test
}

static inline float distance_squared(position a, position b)
{
    float x = crealf(a) - crealf(b),
          y = cimagf(a) - cimagf(b);
    return (x*x) + (y*y);
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
static void check_collisions_against(struct body *us)
{
    struct alloc_bitmap_iterator iter = alloc_bitmap_iterate(bodies);
    struct body *them;
    while((them = iter.next(&iter))) {
        if (us == them || them->flags & COLLIDES_NEVER) continue;
        if ((us->flags & COLLIDES_BY_AFFILIATION ||
             them->flags & COLLIDES_BY_AFFILIATION) &&
            us->affiliation == them->affiliation) continue;
        float d = distance_squared(us->p, them->p);
        float r = maxf(us->collision_radius, them->collision_radius);
        if ((d < r*r) != ((us->flags & COLLIDES_INVERSE) || (them->flags & COLLIDES_INVERSE))) {
            struct collision_msg m = { .base.type = MSG_COLLISION,
                                       .us = us, .them = them };
            TELL(us->ear, &m);
        }
    }
}

static void check_collisions(void)
{
    struct alloc_bitmap_iterator iter = alloc_bitmap_iterate(bodies);
    struct body *b;
    while((b = iter.next(&iter))) {
        if (b->flags & COLLIDES_NEVER || NULL == b->ear)
            continue;
        check_collisions_against(b);
    }
}

void bodies_update(float dt)
{
    struct alloc_bitmap_iterator iter = alloc_bitmap_iterate(bodies);
    struct body *b;
    while((b = iter.next(&iter)))
        body_update(b, dt);

    check_collisions();
}

#ifdef DEBUG
void bodies_foreach(void (*fn)(struct body *))
{
    struct alloc_bitmap_iterator iter = alloc_bitmap_iterate(bodies);
    struct body *b;
    while((b = iter.next(&iter))) (*fn)(b);
}
#endif

#ifdef UNIT_TEST_PHYSICS
#include "libtap/tap.h"
#include <math.h>

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
        b = body_new(random_position(), drand48()/n_bodies);
        if (i%2) body_destroy(b);
    }
    for (unsigned i = 0; i < n_iterations; ++i)
        bodies_update(drand48());
    bodies_destroy();
}

struct collide_testing_ear {
    struct ear base;
    bool collided;
    struct body *body;
};

static enum handler_return basic_collision_test_fn(struct collide_testing_ear *us,
                                            struct msg *m) {
    note("%s called, %p %d", __func__, us, m->type);
    if (m->type != MSG_COLLISION) return STATE_IGNORED;
    us->collided = true;
    return STATE_HANDLED;
}

static void construct_collide_testing_ear(struct collide_testing_ear *a, position p, float r)
{
    *a = (struct collide_testing_ear){
        .base = {.handler = (msg_handler)basic_collision_test_fn},
        .collided = false,
        .body = body_new(p, r) };
    a->body->ear = &a->base;
}

static void test_simple_collision_occurs(void)
{
    struct collide_testing_ear a, b;
    enum handler_return fn(struct collide_testing_ear *us, struct msg *m_) {
        if (m_->type == MSG_COLLISION) {
            struct collision_msg *m = (struct collision_msg *)m_;
            ok(m->us == a.body);
            ok(m->them == b.body);
        }
        return basic_collision_test_fn(us, m_);
    }
    bodies_init(2);
    construct_collide_testing_ear(&a, 0., 10.);
    a.base.handler = (msg_handler)fn;
    construct_collide_testing_ear(&b, 15., 10.);
    bodies_update(1.);
    cmp_ok(a.collided, "==", false);
    for (int i = 5; i > 0; --i) {
        b.body->impulses = -1.;
        bodies_update(1.);
        if (a.collided) goto end;
    }
    fail("Bodies didn't collide when they should have.");
    DUMP_BODY(a.body);
    DUMP_BODY(b.body);
end:
    bodies_destroy();
}

static void test_specific_collision_regression_1(void)
{
    struct collide_testing_ear a, b;
    bodies_init(2);
    construct_collide_testing_ear(&a, 0.135395 + I*0.587962, 0.000003);
    construct_collide_testing_ear(&b, 0.098993 + I*0.551578, 0.000005);
    bodies_update(1.);
    ok(!a.collided && !b.collided);
    bodies_destroy();
}

static void test_specific_collision_regression_2(void)
{
    struct collide_testing_ear a, b;
    bool was_called_ab = false, was_called_ba = false;
    enum handler_return fn(struct ear *us, struct msg *m_) {
        if (m_->type == MSG_COLLISION) {
            struct collision_msg *m = (struct collision_msg *)m_;
            if (m->us == a.body) { ok(m->them == b.body); was_called_ab = true; }
            else if (m->us == b.body) { ok(m->them == a.body); was_called_ba = true; }
            else fail("collision handler incorrectly called");
        }
        return basic_collision_test_fn((struct collide_testing_ear *)us, m_);
    }
    bodies_init(2);
    construct_collide_testing_ear(&a, 0.135395 + I*0.587962, 0.1);
    a.base.handler = fn;
    construct_collide_testing_ear(&b, 0.098993 + I*0.551578, 0.1);
    b.base.handler = fn;
    bodies_update(1.);
    ok(was_called_ab);
    ok(was_called_ba);
    bodies_destroy();
}

static void test_collides_never(void)
{
    bodies_init(2);
    struct collide_testing_ear a;
    construct_collide_testing_ear(&a, 0., 1.);
    struct body *b = body_new(0., 1.);
    b->flags |= COLLIDES_NEVER;
    for (unsigned i = 0; i < 10; ++i)
        bodies_update(drand48());
    bodies_destroy();
    ok(!a.collided);
}


static void test_offside(void)
{
    bodies_init(2);
    // Diagonal length plus a fuzz factor
    float radius = 10. + sqrtf(powf(viewport_w/2, 2)+powf(viewport_h/2,2));
    struct collide_testing_ear a;
    construct_collide_testing_ear(&a, viewport_w/2. + I*(viewport_h/2.), radius);
    a.body->flags |= COLLIDES_INVERSE;
    struct body *b = body_new(0., 10.);
    bodies_update(1.);
    ok(!a.collided);
    int i = 0;
    while (!a.collided && i < 40) {
        b->impulses = -5.;
        bodies_update(1.);
        ++i;
    }
    bodies_destroy();
    cmp_ok(i, "==", 3);
}

static void test_affiliation(void)
{
    bodies_init(2);
    body_new(0., 1.);
    struct collide_testing_ear b;
    construct_collide_testing_ear(&b, 0., 1.);
    b.body->flags |= COLLIDES_BY_AFFILIATION;
    bodies_update(1.);
    ok(!b.collided);
    b.body->affiliation = 1;
    bodies_update(1.);
    ok(b.collided);
    bodies_destroy();
}

static void test_collision_flags(void)
{
    test_collides_never();
    test_affiliation();
    test_offside();
}

int main(void)
{
    plan(14);
    long seed = time(NULL);
    note("srand48(%ld)\n", seed);
    srand48(seed);
    test_specific_collision_regression_1();
    test_specific_collision_regression_2();
    test_simple_collision_occurs();
    test_collision_flags();
    lives_ok({simple_test(1000, 100);});
    done_testing();
}
#endif
