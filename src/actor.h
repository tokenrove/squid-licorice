#pragma once

#include "msg.h"
#include "sprite.h"
#include "physics.h"
#include "game_constants.h"

struct actor {
    struct ear base;
    struct sprite sprite;
    struct body *body;
    void *state;
};

struct archetype {
    const char *atlas_path;
    float collision_radius, mass;
    msg_handler initial_handler;
    size_t state_size;
};

extern void actors_init(size_t n, struct archetype *archetypes, size_t n_archetypes);
extern void actors_destroy(void);
extern void actors_draw(void);
extern void actors_update(float elapsed_time);

extern struct actor *actor_spawn(enum actor_archetype type, position p, void *state);
