#pragma once

#include <stdlib.h>
#include "geometry.h"
#include <stdint.h>

enum collision_flags {
    COLLIDES_NEVER          = 1, // if set, all collision is bypassed
    COLLIDES_BY_AFFILIATION = 2, // if set, objects of other affiliation are ignored
    COLLIDES_INVERSE        = 4  // reverse the result of the collision test
};

struct body {
    position p, v, F, impulses;
    void (*collision_fn)(struct body *us, struct body *them, void *data);
    void *data;
    float collision_radius, mass;
    uint16_t affiliation;
    enum collision_flags flags;
};

extern void bodies_init(size_t n);
extern void bodies_destroy(void);
extern void bodies_update(float dt);

extern struct body *body_new(position p, float collision_radius);
extern void body_destroy(struct body *body);
