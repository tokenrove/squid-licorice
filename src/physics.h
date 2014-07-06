#pragma once

#include <stdlib.h>
#include <stdint.h>
#include "geometry.h"
#include "msg.h"

enum collision_flags {
    COLLIDES_NEVER          = 1, // if set, all collision is bypassed
    COLLIDES_BY_AFFILIATION = 2, // if set, objects of other affiliation are ignored
    COLLIDES_INVERSE        = 4  // reverse the result of the collision test
};

struct body {
    position p, v, F, impulses;
    float collision_radius, mass;
    uint8_t affiliation;
    enum collision_flags flags;
    struct ear *ear;
};

struct collision_msg {  /* MSG_COLLISION */
    struct msg base;
    struct body *us, *them;
};

extern void bodies_init(size_t n);
extern void bodies_destroy(void);
extern void bodies_update(float dt);
#ifdef DEBUG
extern void bodies_foreach(void (*fn)(struct body *));
#endif

extern struct body *body_new(position p, float collision_radius);
extern void body_destroy(struct body *body);
