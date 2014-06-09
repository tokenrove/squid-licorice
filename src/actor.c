
#include "actor.h"
#include "ensure.h"
#include "alloc_bitmap.h"

#define EV_SUPER(super_) (me->handler = super_, STATE_SUPER)

struct player {
    // weapons etc
};

static enum handler_return player_initial(struct actor *me, struct event *e)
{
    return STATE_IGNORED;
}

struct archetype {
    const char *atlas_path;
    float collision_radius, mass;
    state_handler initial_handler;
    size_t state_size;
} archetypes[] = {
    { .atlas_path = "player.png",
      .collision_radius = 20,
      .mass = 30,
      .initial_handler = player_initial,
      .state_size = sizeof (struct player)
    },
};

bool actor_signal(struct actor *actor, struct event *event)
{
    state_handler s, t;
    enum handler_return r;

    t = actor->handler;
    do {
        s = actor->handler;
        if (NULL == s) return false;
        r = (*s)(actor, event);
    } while (STATE_SUPER == r);
    if (STATE_TRANSITION != r) return true;

    struct event exit = {.signal = SIGNAL_EXIT},
                enter = {.signal = SIGNAL_ENTER};
    (*t)(actor, &exit);
    if (NULL == actor->handler) return false;
    r = (actor->handler)(actor, &enter);
    return (NULL != actor->handler);
}

static alloc_bitmap actors;

void actors_init(size_t n)
{
    ENSURE(actors = alloc_bitmap_init(n, sizeof (struct actor)));
}

void actors_destroy(void)
{
    alloc_bitmap_destroy(actors);
    actors = NULL;
}

void actors_draw(void)
{
    struct actor *a;
    struct alloc_bitmap_iterator iter = alloc_bitmap_iterate(actors);
    while ((a = (struct actor *)iter.next(&iter)))
        sprite_draw(&a->sprite, a->body->p);
}

static void destroy(struct actor *a)
{
    body_destroy(a->body);
    texture_destroy(a->sprite.atlas);
    free(a->sprite.atlas);
}

void actors_update(float elapsed_time)
{
    struct actor *a;
    struct alloc_bitmap_iterator iter = alloc_bitmap_iterate(actors);
    struct tick_event tick = {.super.signal = SIGNAL_TICK, .elapsed_time = elapsed_time};
    while((a = iter.next(&iter))) {
        actor_signal(a, (struct event *)&tick);
        if (!a->handler) {
            destroy(a);
            iter.mark_for_removal(&iter);
        }
    }
    iter.expunge_marked(&iter);
}

struct actor *actor_spawn_at_position(enum actor_archetype type, position p)
{
    ENSURE(type < ARCHETYPE_LAST);
    struct archetype *arch = &archetypes[type];
    struct actor *a = (struct actor *)alloc_bitmap_alloc_first_free(actors);
    *a = (struct actor){
        .handler = arch->initial_handler
    };
    a->sprite = (struct sprite) { .x = 0, .y = 0, .scaling = 1.f, .rotation = 0. };
    // XXX will go in a dedicated atlas cache somewhere instead
    ENSURE(a->sprite.atlas = calloc(1, sizeof (struct texture)));
    ENSURE(texture_from_png(a->sprite.atlas, arch->atlas_path));
    a->sprite.w = a->sprite.atlas->width;
    a->sprite.h = a->sprite.atlas->height;
    ENSURE(a->body = body_new(p, arch->collision_radius, arch->mass));
    return a;
}
