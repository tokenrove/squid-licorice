#pragma once

#include <stdlib.h>
#include "geometry.h"

struct body {
    position p, v, F;
    float collision_radius, mass;
};

extern void bodies_init(size_t n);
extern void bodies_destroy(void);

extern struct body *body_new(position p, float collision_radius, float mass);
extern void body_destroy(struct body *body);
