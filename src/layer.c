#include "video.h"
#include "camera.h"
#include <stdlib.h>
#include <string.h>
#include "layer.h"
#include "ensure.h"

enum { RING_SIZE = 16, RING_MASK = 0xf };  /* arbitrary guesstimate */

struct layer *layer_new(int direction, struct tilemap *(*fn)(void *), void *ctx)
{
    struct layer *t = calloc(1, sizeof (*t));
    ENSURE(t);
    ENSURE(direction == SCROLL_UP || direction == SCROLL_DOWN);
    t->direction = direction;
    if (direction == SCROLL_UP)
        t->position = viewport_h;
    t->next_fn = fn;
    t->next_fn_context = ctx;

    ENSURE(t->ring = calloc(RING_SIZE, sizeof(*t->ring)));
    t->first = 0;
    t->last = 0;
    return t;
}

void layer_destroy(struct layer *t)
{
    free(t->ring);
    memset(t, 0, sizeof (*t));
    free(t);
}

void layer_update(struct layer *t, float elapsed_time)
{
    // XXX check if there's a scroll limit; if so, stop
    t->position += elapsed_time * t->speed;
}

static inline int succ(int i) { return (i + 1) & RING_MASK; }

static void request_another_chunk(struct layer *t)
{
    int i = t->last;
    t->ring[i] = t->next_fn(t->next_fn_context);
    ENSURE(t->ring[i]);
    t->last = succ(i);
}

static void ensure_ith_chunk(struct layer *t, unsigned i)
{
    if (i == t->last)
        request_another_chunk(t);
    ENSURE(t->ring[i]);
}

static void find_the_lowest_visible_chunk(struct layer *t, float bottom)
{
    unsigned i = t->first;
    do {
        ensure_ith_chunk(t, i);
        if (t->position-t->ring[i]->map.h < bottom) break;
        t->position -= t->ring[i]->map.h;
        i = succ(i);
    } while(true);
    t->first = i;
}

static void draw_up_while_space_available(struct layer *t, float top)
{
    unsigned i = t->first;
    for(float p = t->position; p > top; i = succ(i)) {
        ensure_ith_chunk(t, i);
        p -= t->ring[i]->map.h;
        tilemap_draw(t->ring[i], I*p);
    }
}

static void draw_bottom_up(struct layer *t, float top, float bottom)
{
    find_the_lowest_visible_chunk(t, bottom);
    draw_up_while_space_available(t, top);
}

void layer_draw(struct layer *t)
{
    float camera_top = cimagf(world_camera.translation),
       camera_bottom = viewport_h + camera_top;

    switch (t->direction) {
    case SCROLL_UP:
        draw_bottom_up(t, camera_top, camera_bottom);
        return;
    case SCROLL_DOWN:
        //draw_top_to_bottom(t, camera_top, camera_bottom);
        return;
    default:
        ENSURE(false);
    }
}

#ifdef UNIT_TEST_LAYER
#include "libtap/tap.h"

static void test_layer_with_one_slice(void)
{
    struct tilemap slice;
    bool fn_was_called = false;
    struct tilemap *fn(void *_ __attribute__ ((unused))) {
        fn_was_called = true;
        return &slice;
    }

    ENSURE(tilemap_load("t/layer.t-0.0.map", "t/layer.t-0.0.png", &slice));

    struct layer *layer = layer_new(SCROLL_UP, fn, NULL);
    ok(NULL != layer);
    ok(!fn_was_called);
    for (int i = 0; i < 3; ++i) {
        layer_update(layer, 1.);
        video_start_frame();
        layer_draw(layer);
        video_end_frame();
    }
    ok(fn_was_called);
    layer_destroy(layer);
}

int main(void)
{
    video_init();
    camera_init();
    tilemap_init();
    plan(3);
    test_layer_with_one_slice();
    done_testing();
}
#endif
