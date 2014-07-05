#pragma once

#include <stdlib.h>
#include "geometry.h"

struct body {
    position p, v, F, impulses;
    float collision_radius, mass;
    void (*collision_fn)(struct body *us, struct body *them, void *data);
    void *data;
};

extern void bodies_init(size_t n);
extern void bodies_destroy(void);
extern void bodies_update(float dt);

extern struct body *body_new(position p, float collision_radius, float mass);
extern void body_destroy(struct body *body);
