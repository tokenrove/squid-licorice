#pragma once

#include "sprite.h"
#include "physics.h"

enum actor_archetype {
    ARCHETYPE_PLAYER1,
    ARCHETYPE_WAVE_ENEMY,
    ARCHETYPE_CIRCLING_ENEMY,
    ARCHETYPE_LAST
};

typedef enum {
    SIGNAL_ENTER,
    SIGNAL_EXIT,
    SIGNAL_TICK,
    SIGNAL_COLLISION
} signal;

struct event {
    signal signal;
};

struct tick_event {
    struct event super;
    float elapsed_time;
};

enum handler_return { STATE_SUPER, STATE_TRANSITION, STATE_HANDLED, STATE_IGNORED };

struct actor;
typedef enum handler_return (*state_handler)(struct actor *, struct event *);

struct actor {
    state_handler handler;
    struct sprite sprite;
    struct body *body;
    void *state;
};

extern void actors_init(size_t n);
extern void actors_destroy(void);
extern void actors_draw(void);
extern void actors_update(float elapsed_time);

extern struct actor *actor_spawn_at_position(enum actor_archetype type, position p);
// Returns whether this actor is still alive (true).
extern bool actor_signal(struct actor *actor, struct event *event);
