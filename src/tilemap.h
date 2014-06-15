#pragma once

#include <stdbool.h>

#include "geometry.h"
#include "texture.h"

struct tilemap {
    struct {
        float w, h;
        struct texture texture;
    } atlas, map;
};

extern void tilemap_init(void);
extern bool tilemap_load(const char *map_path, const char *atlas_path, struct tilemap *t);
extern void tilemap_draw(struct tilemap *t, position offset);
extern void tilemap_destroy(struct tilemap *t);
