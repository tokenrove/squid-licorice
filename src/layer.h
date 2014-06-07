#pragma once

#include "tilemap.h"

struct layer {
    struct tilemap *(*next_fn)(void *);
    void *next_fn_context;

    enum { SCROLL_UP, SCROLL_DOWN } direction;
    float speed;

    struct tilemap **ring;
    unsigned first, last;
    float position;
};

enum { MINIMUM_CHUNK_HEIGHT = 16*8 };  /* Tilemaps must be at least 8 tiles high */

extern struct layer *layer_new(int direction, struct tilemap *(*fn)(void *), void *ctx);
extern void layer_destroy(struct layer *t);
extern void layer_update(struct layer *t, float elapsed_time);
extern void layer_draw(struct layer *t);


