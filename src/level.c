
#include "tilemap.h"
#include <stdlib.h>

#include "level.h"
#include "ensure.h"
#include "strand.h"
#include "stage.h"
#include "video.h"
#include "actor.h"

#include <SDL2/SDL.h>


static void wait_for_n_screens(strand self, struct layer *layer, float n_screens)
{
    double distance = 0.;
    float elapsed_time;
    while (distance < (viewport_h * n_screens)) {
        elapsed_time = strand_yield(self);
        distance += layer->speed * elapsed_time;
    }
}

static void wait_for_elapsed_time(strand self, float goal)
{
    double accum = 0.;
    while (accum < goal)
        accum += strand_yield(self);
}

static void ease_to_scroll_speed(strand self, struct layer *layer, float final,
                                 float duration, easing_fn easing)
{
    float initial = layer->speed;
    float elapsed = 0.f;

    while (elapsed < duration) {
        elapsed += strand_yield(self);
        float w = easing(elapsed, duration);
        layer->speed = initial + (final - initial) * w;
    }
    layer->speed = final;
}

struct next_context {
    int count;
};

static void level1_entry(strand self)
{
    struct tilemap slices[3];
    struct next_context context = {0};

    struct tilemap *next(void *data) {
        struct next_context *ctx = data;
        switch (ctx->count++) {
        case 0: return &slices[0];
        case 1: return &slices[1];
        default: return &slices[2];
        };
    };

    // load chunks
    ENSURE(tilemap_load("slice1a.map", "slice1a.png", &slices[0]));
    ENSURE(tilemap_load("slice1b.map", "slice1b.png", &slices[1]));
    ENSURE(tilemap_load("slice1c.map", "slice1c.png", &slices[2]));

    struct layer *main_layer = layer_new(SCROLL_UP, next, &context);
    stage_add_layer(main_layer);

    // actors
    // spawn player
    struct actor *player = actor_spawn(ARCHETYPE_PLAYER1, viewport_w/2 + I*(viewport_h-50.f), NULL);
    ENSURE(player);

    strand_yield(self);
    /* we're finished setting up */

    ease_to_scroll_speed(self, main_layer, 200.f, 3., easing_cubic);
    wait_for_n_screens(self, main_layer, 1);

    // group 1
    {
        unsigned group_ctr = 0;
        struct { unsigned *group_ctr; } enemy_aux = { .group_ctr = &group_ctr };
        struct actor *enemies[2];
        enemies[0] = actor_spawn(ARCHETYPE_WAVE_ENEMY, viewport_w/2. + I*50.f, &enemy_aux);
        enemies[1] = actor_spawn(ARCHETYPE_WAVE_ENEMY, viewport_w/2. - 100.f + I*50.f, &enemy_aux);
        ++group_ctr;
        ease_to_scroll_speed(self, main_layer, 0.f, 6., easing_cubic);
        while (group_ctr > 0) strand_yield(self);
    }

    wait_for_elapsed_time(self, 10.);

    layer_destroy(main_layer);
    for (int i = 0; i < 3; ++i)
        tilemap_destroy(&slices[i]);
}


static struct {
    void (*fn)(strand);
    size_t fn_stack_size;
} levels[N_LEVELS] = {
    { .fn = level1_entry, .fn_stack_size = STRAND_DEFAULT_STACK_SIZE }
};

struct level *level_load(unsigned i)
{
    stage_start();
    struct level *level;
    ENSURE(i < N_LEVELS);
    ENSURE(level = calloc(1, sizeof (*level)));
    level->strand = strand_spawn_0(levels[i].fn, levels[i].fn_stack_size);
    return level;
}

void level_destroy(struct level *level)
{
    stage_end();
    strand_destroy(level->strand);
    free(level);
}
