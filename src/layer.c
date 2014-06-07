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

static void draw_bottom_up(struct layer *t, float top, float bottom)
{
    float p = t->position;
    unsigned i = t->first;

    // get to the bottom of the screen
    do {
        if (i == t->last)
            request_another_chunk(t);
        ENSURE(t->ring[i]);
        if (p-t->ring[i]->map.h < bottom) break;    /* p is now visible */
        p -= t->ring[i]->map.h;  /* move p up one chunk */
        t->position -= t->ring[i]->map.h;
        i = succ(i);               /* next chunk */
    } while(true);
    t->first = i;  /* this is the new first chunk */

    // draw while there's visible space left
    do {
        if (i == t->last)
            request_another_chunk(t);
        ENSURE(t->ring[i]);
        if (p <= top) break;
        p -= t->ring[i]->map.h;
        tilemap_draw(t->ring[i], I*p);
        i = succ(i);
    } while(true);
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

/* 
 * void camera_scroll(float amount_px)
 * {
 *     world_camera.translation += elapsed_time * scroll_speed;
 *     camera.bottom += amount_px;
 * }
 */


/* 
 * void map_advance(void)
 * {
 *     int i, j;
 * 
 *     float camera_top = world_camera.translation,
 *        camera_bottom = viewport_h + camera_top;
 * 
 *     for (i = 0; chunks[i].t && chunks[i].p + chunks[i].t->map.h < camera_top);
 * 
 *     // XXX memcpy
 *     for (j = 0; chunks[i].t; ++j, ++i) chunks[j] = chunks[i];
 * 
 *     for (; 0 == j || chunks[j-1].bottom < camera_top; ++j) {
 *         ENSURE(j < n_chunks);
 *         chunks[j] = chunks[0];
 *         chunks[j].bottom = chunks[j-1].bottom + chunks[j-1].t->map.h;
 *         chunks[j].top = chunks[j].bottom + chunks[j].height;
 *         if (j == 0) signal_next_chunk(); /\* XXX wait, is that right? *\/
 *     }
 *     chunks[j] = NULL;
 *     for (i = 0; chunks[i]; ++i)
 *         render(chunks[i], chunks[i].bottom - camera.y);
 * }
 */
