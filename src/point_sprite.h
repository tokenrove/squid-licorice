#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "geometry.h"
#include "texture.h"

struct point_sprite {
    struct texture *atlas;
    uint16_t x,y,size;
};

extern void point_sprite_init(void);
extern void point_sprite_draw_batch(struct point_sprite *s, size_t n, position *ps);
