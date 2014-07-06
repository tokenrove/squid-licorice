
#include "actor.h"
#include "log.h"
#include "ensure.h"
#include "alloc_bitmap.h"
#include "msg_macros.h"

static struct archetype *archetypes;
static size_t n_archetypes;
static alloc_bitmap actors;

void actors_init(size_t n, struct archetype *as, size_t n_as)
{
    ENSURE(archetypes = as);
    n_archetypes = n_as;
    ENSURE(n_archetypes > 0);
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
    struct tick_msg tick = {.super.type = MSG_TICK, .elapsed_time = elapsed_time};
    while((a = iter.next(&iter))) {
        TELL(a, &tick);
        if (!a->base.handler) {
            destroy(a);
            iter.mark_for_removal(&iter);
        }
    }
    iter.expunge_marked(&iter);
}

struct actor *actor_spawn(enum actor_archetype type, position p, void *state)
{
    ENSURE(type < n_archetypes);
    struct archetype *arch = &archetypes[type];
    struct actor *a = (struct actor *)alloc_bitmap_alloc_first_free(actors);
    if (NULL == a) return NULL;
    *a = (struct actor){
        .base.handler = arch->initial_handler,
        .state = state
    };
    a->sprite = (struct sprite) { .x = 0, .y = 0, .scaling = 1.f, .rotation = 0. };
    // XXX will go in a dedicated atlas cache somewhere instead
    ENSURE(a->sprite.atlas = calloc(1, sizeof (struct texture)));
    // XXX should use a placeholder if texture fails to load
    ENSURE(texture_from_png(a->sprite.atlas, arch->atlas_path));
    a->sprite.w = a->sprite.atlas->width;
    a->sprite.h = a->sprite.atlas->height;
    ENSURE(a->body = body_new(p, arch->collision_radius));
    a->body->mass = arch->mass;
    a->body->ear = &a->base;
    struct msg enter = { .type = MSG_ENTER };
    TELL(a, &enter);
    return a;
}

#ifdef UNIT_TEST_ACTOR
#include "libtap/tap.h"
#include "video.h"
#include "camera.h"
#include "physics.h"

static enum handler_return
do_nothing_handler(struct ear *me __attribute__((unused)),
                   struct msg *m __attribute__((unused))) {
    return STATE_IGNORED;
}

enum { N_TEST_ARCHETYPES = 1 };
static struct archetype test_archetypes[N_TEST_ARCHETYPES] = {
    { .atlas_path = "t/common.tiny-1x1.png",
      .collision_radius = 20.,
      .mass = 30.,
      .initial_handler = (msg_handler)do_nothing_handler,
      .state_size = 0 }
};

static void test_actors_basic_api()
{
    note("Test basic API on actors for ordinary, successful cases");
    int n = 32;
    bodies_init(n);
    actors_init(n, test_archetypes, N_TEST_ARCHETYPES);
    /* draw no one */
    actors_draw();
    actors_update(1.);

    struct actor *a = actor_spawn(0, 0., NULL);
    for (int i = 1; i < n; ++i)
        ok(NULL != actor_spawn(0, 0., NULL));
    // See what happens at the limits.  n should be a power of 2.
    ok(NULL == actor_spawn(0, 0., NULL));

    actors_draw();
    actors_update(1.);

    struct tick_msg tick = { .super.type = MSG_TICK, .elapsed_time = 1. };
    TELL(a, &tick.super);
    ok(NULL != a->base.handler);

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
    plan(33);
    test_actors_basic_api();
    // TODO verify a placeholder sprite is used if texture fails to load
    // TODO verify a placeholder actor is used if archetype doesn't exist
    // TODO construct an alternate archetypes table for testing
    done_testing();
}
#endif
