
#include "tilemap.h"
#include <stdlib.h>

#include "level.h"
#include "ensure.h"
#include "strand.h"
#include "stage.h"
#include "video.h"

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
    struct tilemap slice1a, slice1b, slice1c;
    struct next_context context = {0};

    struct tilemap *next(void *data) {
        struct next_context *ctx = data;
        switch (ctx->count++) {
        case 0: return &slice1a;
        case 1: return &slice1b;
        default: return &slice1c;
        };
    };

    // load chunks
    ENSURE(tilemap_load("slice1a.map", "slice1a.png", &slice1a));
    ENSURE(tilemap_load("slice1b.map", "slice1b.png", &slice1b));
    ENSURE(tilemap_load("slice1c.map", "slice1c.png", &slice1c));

    struct layer *main_layer = layer_new(SCROLL_UP, next, &context);
    stage_add_layer(main_layer);
    ease_to_scroll_speed(self, main_layer, 200.f, 3., easing_cubic);
    wait_for_n_screens(self, main_layer, 3);
    ease_to_scroll_speed(self, main_layer, 0.f, 6., easing_cubic);
    wait_for_elapsed_time(self, 10.);

    layer_destroy(main_layer);
    tilemap_destroy(&slice1a);
    tilemap_destroy(&slice1b);
    tilemap_destroy(&slice1c);
}


static struct {
    void (*fn)(strand);
    size_t fn_stack_size;
} levels[N_LEVELS] = {
    { .fn = level1_entry, .fn_stack_size = STRAND_DEFAULT_STACK_SIZE }
};

struct level *level_load(int i)
{
    stage_start();
    struct level *level;
    level = calloc(1, sizeof (*level));
    ENSURE(level);
    level->strand = strand_spawn(levels[i].fn, levels[i].fn_stack_size);
    return level;
}

void level_destroy(struct level *level)
{
    stage_end();
    strand_destroy(level->strand);
    free(level);
}
