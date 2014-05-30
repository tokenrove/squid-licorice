#pragma once

struct texture {
    uint16_t width, height;  /* in pixels */
    GLuint id;
};

extern void texture_init(void);
extern struct texture texture_from_png(const char *path);
