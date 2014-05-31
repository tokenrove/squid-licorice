#pragma once

#include <stdbool.h>
#include <GL/gl.h>

struct texture {
    uint16_t width, height;  /* in pixels */
    GLuint id;
};

extern void texture_init(void);
extern bool texture_from_pels(struct texture *t, uint8_t *pels,
                              uint16_t w, uint16_t h, uint8_t bpp);
extern bool texture_from_png(struct texture *dest, const char *path);
extern void texture_destroy(struct texture *t);
