
#include "physics.h"
#include "alloc_bitmap.h"
#include "ensure.h"
#include "video.h"
#include "inline_math.h"

static alloc_bitmap bodies;
static size_t queue_max;
static struct body_event *queue;
static size_t queue_p;

static inline size_t maxu(size_t a, size_t b) { return (a > b) ? a : b; }

void bodies_init(size_t n)
{
    ENSURE(bodies = alloc_bitmap_init(n, sizeof (struct body)));
    queue_max = maxu(32, 1+n);
    ENSURE(queue = calloc(queue_max, sizeof (struct body_event)));
    queue_p = 0;
}

void bodies_destroy(void)
{
    alloc_bitmap_destroy(bodies);
    bodies = NULL;
    free(queue);
    queue = NULL;
    queue_p = 0;
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
    // XXX remove references to this in the queue?  write a test
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

static void push(struct body_event ev)
{
    queue[queue_p] = ev;
    ++queue_p;
    ENSURE(queue_p < queue_max);
}

static void push_collision_event(struct body *a, struct body *b)
{
    struct body_event ev = (struct body_event){0};
    ev.type = BODY_COLLISION;
    ev.a = a;
    ev.b = b;
    push(ev);
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
        if (a == b) continue;
        double d = cabsf(a->p - b->p);
        double r = maxf(a->collision_radius, b->collision_radius);
        if (d*d < r*r)
            push_collision_event(a, b);
    }
}

static void push_offside_event(struct body *offender)
{
    struct body_event ev = (struct body_event){0};
    ev.type = BODY_OFFSIDE;
    ev.a = offender;
    push(ev);
}

static void apply_offside_rule(struct body *b)
{
    if (crealf(b->p) < -b->collision_radius/2 ||
        crealf(b->p) > viewport_w + b->collision_radius/2 ||
        cimagf(b->p) < -b->collision_radius/2 ||
        cimagf(b->p) > viewport_h + b->collision_radius/2)
        push_offside_event(b);
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
    queue_p = 0;

    struct alloc_bitmap_iterator iter = alloc_bitmap_iterate(bodies);
    struct body *b;
    while((b = iter.next(&iter)))
        body_update(b, dt);

    check_collisions(dt);
}

struct body_event *bodies_poll(void)
{
    if (queue_p > 0)
        return &queue[--queue_p];
    return NULL;
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
    bodies_init(2);
    struct body *a, *b;
    a = body_new(0., 10., 1.);
    ok(NULL != a);
    b = body_new(15., 10., 1.);
    ok(NULL != b);
    bodies_update(1.);
    ok(NULL == bodies_poll());
    for (int i = 5; i > 0; --i) {
        b->impulses = -1.;
        bodies_update(1.);
        struct body_event *ev;
        while ((ev = bodies_poll())) {
            cmp_ok(ev->type, "==", BODY_COLLISION);
            if (ev->a == a)     ok(ev->b == b);
            else if(ev->a == b) ok(ev->b == a);
            else                fail("Arguments of event don't match bodies we have.");
            goto end;
        }
    }
    fail("Bodies didn't collide when they should have.");
    DUMP_BODY(a);
    DUMP_BODY(b);
end:
    bodies_destroy();
}

static void test_specific_collision_regression(void)
{
    bodies_init(2);
    body_new(0.135395 + I*0.587962, 0.000003, 1.);
    body_new(0.098993 + I*0.551578, 0.000005, 1.);
    bodies_update(1.);
    ok(NULL == bodies_poll());
    bodies_destroy();
}

int main(void)
{
    plan(7);
    long seed = time(NULL);
    note("srand48(%ld)\n", seed);
    srand48(seed);
    test_specific_collision_regression();
    test_simple_collision_occurs();
    lives_ok({simple_test(1000, 100);});
    done_testing();
}
#endif
