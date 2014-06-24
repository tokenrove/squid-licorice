
#include "physics.h"
#include "alloc_bitmap.h"
#include "ensure.h"

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

void bodies_update(float dt)
{
    struct alloc_bitmap_iterator iter = alloc_bitmap_iterate(bodies);
    struct body *b;
    while((b = iter.next(&iter)))
        body_update(b, dt);
}

#ifdef UNIT_TEST_PHYSICS
#include "libtap/tap.h"

int main(void)
{
    plan(1);
    todo();
    pass();
    end_todo;
    done_testing();
}
#endif
