#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "geometry.h"
#include "texture.h"

struct sprite {
    struct texture *atlas;
    uint16_t x,y,w,h;
    bool all_white;
    float scaling, rotation;
};

extern void sprite_init(void);
extern void sprite_draw(struct sprite *s, position p);
