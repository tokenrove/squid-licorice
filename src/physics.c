
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
