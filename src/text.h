#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "geometry.h"
#include "texture.h"

struct glyph {
    uint32_t char_code;         /* UCS4 character code */
    uint16_t x, y;
    uint8_t w, h;
    int8_t x_offset, y_offset, x_advance;
};

/*
 * The font structure keeps track of the glyphs in sorted order.
 */
struct font {
    uint16_t n_glyphs, line_height;
    struct glyph *glyphs;
    struct texture texture;
    int8_t printable_ascii_lookup[128-32]; /* Lookup table for characters from 32 to 127 */
};

extern void text_init(void);
extern bool text_load_font(struct font *font, const char *path);
extern void text_destroy_font(struct font *font);
/**
 * font: loaded font to use
 * p: position on screen
 * color: RGBA color of text
 * s: UTF-8, NUL-terminated string to render
 */
extern void text_render_line(struct font *font, position p, uint32_t color, const char *s);
extern void text_render_line_with_shadow(struct font *font, position p, uint32_t color, const char *s);
extern uint16_t text_get_line_height(struct font *font);
extern uint16_t text_width(struct font *font, const char *s);
