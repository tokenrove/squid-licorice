#pragma once

#include <stdbool.h>

#include "texture.h"

struct tilemap {
    struct texture atlas_texture;
    GLuint map_texture, shader, a_vertices;
    float dims[4];
};

extern void tilemap_init(void);
extern bool tilemap_load(char *map_path, char *atlas_path, struct tilemap *t);
extern void tilemap_draw(struct tilemap *t, float *mv_matrix, float *projection_matrix);
extern void tilemap_destroy(struct tilemap *t);
