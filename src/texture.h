#pragma once

#include <stdbool.h>
#include <GL/gl.h>

struct texture {
    uint16_t width, height;  /* in pixels */
    GLuint id;
};

extern void texture_init(void);
extern bool texture_from_png(struct texture *dest, const char *path);
