#pragma once

#include <stdlib.h>
#include "geometry.h"

struct body {
    position p, v, F, impulses;
    float collision_radius, mass;
};

struct body_event {
    enum { BODY_COLLISION, BODY_OFFSIDE } type;
    struct body *a, *b;
};

extern void bodies_init(size_t n);
extern void bodies_destroy(void);
extern void bodies_update(float dt);

extern struct body_event *bodies_poll(void);

extern struct body *body_new(position p, float collision_radius, float mass);
extern void body_destroy(struct body *body);
