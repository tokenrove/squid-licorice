
#include "log.h"
#include "actor.h"
#include "ensure.h"
#include "alloc_bitmap.h"
#include "input.h"

#define EV_SUPER(super_) (me->handler = super_, STATE_SUPER)

struct player {
    // weapons etc
};

static enum handler_return player_default(struct actor *me, struct event *e)
{
    switch (e->signal) {
    case SIGNAL_TICK:
        if (inputs[IN_UP])
            me->body->impulses += -5.*inputs[IN_UP]*I;
        if (inputs[IN_DOWN])
            me->body->impulses += 5.*inputs[IN_DOWN]*I;
        if (inputs[IN_LEFT])
            me->body->impulses += -5.*inputs[IN_LEFT];
        if (inputs[IN_RIGHT])
            me->body->impulses += 5.*inputs[IN_RIGHT];
        break;
    default: break;
    }
    return STATE_IGNORED;
}

static enum handler_return player_initial(struct actor *me, struct event *e __attribute__((__unused__)))
{
    me->sprite.x = 0;
    me->sprite.y = 0;
    me->sprite.w = 29;
    me->sprite.h = 51;
    return me->handler = player_default, STATE_TRANSITION;
}

struct enemy_a {
    unsigned *group_ctr;
};

static enum handler_return enemy_a_initial(struct actor *me, struct event *e)
{
    me->sprite.x = 58;
    me->sprite.y = 0;
    me->sprite.w = 26;
    me->sprite.h = 24;
    switch (e->signal) {
    case SIGNAL_TICK: break;
    case SIGNAL_OFFSIDE:
        LOG("%p went offside", me);
        break;
    default: break;
    }
    return STATE_IGNORED;
}

struct archetype {
    const char *atlas_path;
    float collision_radius, mass;
    state_handler initial_handler;
    size_t state_size;
} archetypes[] = {
    { .atlas_path = "sprites.png",
      .collision_radius = 20,
      .mass = 30,
      .initial_handler = player_initial,
      .state_size = sizeof (struct player)
    },
    { .atlas_path = "sprites.png",
      .collision_radius = 20,
      .mass = 30,
      .initial_handler = enemy_a_initial,
      .state_size = sizeof (struct enemy_a)
    }
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

struct actor *actor_spawn(enum actor_archetype type, position p, void *state)
{
    ENSURE(type < ARCHETYPE_LAST);
    struct archetype *arch = &archetypes[type];
    struct actor *a = (struct actor *)alloc_bitmap_alloc_first_free(actors);
    *a = (struct actor){
        .handler = arch->initial_handler,
        .state = state
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

#ifdef UNIT_TEST_ACTOR
#include "libtap/tap.h"
#include "video.h"
#include "camera.h"
#include "physics.h"

static void test_actors_basic_api()
{
    int n = 32;
    bodies_init(n);
    actors_init(n);
    /* draw no one */
    actors_draw();
    actors_update(1.);
    struct actor *a = actor_spawn(ARCHETYPE_WAVE_ENEMY, 0., NULL);
    for (int i = 1; i < n; ++i)
        ok(NULL != actor_spawn(ARCHETYPE_WAVE_ENEMY, 0., NULL));
    ok(NULL == actor_spawn(ARCHETYPE_WAVE_ENEMY, 0., NULL));
    actors_draw();
    actors_update(1.);
    struct tick_event tick = { .super = { .signal = SIGNAL_TICK },
                               .elapsed_time = 1. };
    ok(actor_signal(a, (struct event *)&tick));
    actors_draw();
    actors_update(1.);
    actors_destroy();
    bodies_destroy();
}

int main(void)
{
    video_init();
    camera_init();
    sprite_init();
    plan(1);
    test_actors_basic_api();
    done_testing();
}
#endif
