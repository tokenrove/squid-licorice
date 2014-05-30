
/*
 * We work with a single bitmap font in terms of its originally
 * rendered size, and only deal with scaling factors at render time.
 * Obviously having a separate font rendered at each size would look
 * better (or using a TTF renderer directly), but this is a useful
 * compromise.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <GL/gl.h>

#include "text.h"
#include "ensure.h"

#define LOG(message, ...) do {                 \
        fprintf(stderr, "%s: ", __func__);     \
        fprintf(stderr, message, __VA_ARGS__); \
        fputc('\n', stderr);                   \
    } while(0)

/*
 * path: NUL-terminated string containing the path to the font file
 * without extension (foo passed in loads foo.fnt and foo.png)
 */
bool text_load_font(struct font *font, const char *path)
{
    enum { BUFLEN = 127 };
    char buf[BUFLEN+1];
    FILE *fp;

    *font = (struct font){0};

    strncpy(buf, path, BUFLEN);
    strncat(buf, ".fnt", BUFLEN);
    fp = fopen(buf, "rb");
    if (NULL == fp) {
        LOG("file not found (%s)", buf);
        return false;
    }
    bool r;
    fgets(buf, BUFLEN, fp);  // skip first line
    while (!feof(fp)) {
        struct glyph *glyph;
        if (NULL == fgets(buf, BUFLEN, fp)) break;
        r = false;
        ++font->n_glyphs;
        // XXX next font format should declare the number of glyphs up front
        font->glyphs = realloc(font->glyphs, font->n_glyphs * sizeof (*font->glyphs));
        if (NULL == font->glyphs) {
            LOG("Failed to allocate memory for glyphs (%s.fnt)", path);
            break;
        }

        if ('\n' != buf[strlen(buf)-1]) {
            LOG("Sorry, we expect every font file line to end in an LF (%s.fnt)", path);
            break;
        }
        // Also y_advance at the end of the line, but we ignore it.
        glyph = &font->glyphs[font->n_glyphs-1];
        if (8 != sscanf(buf, "%u %hhd %hhd %hhd %hhd %hhd %hhd %hhd",
                        &glyph->char_code,
                        &glyph->x, &glyph->y, &glyph->w, &glyph->h,
                        &glyph->x_offset, &glyph->y_offset, &glyph->x_advance)) {
            LOG("Didn't get exactly eight values; misformatted font file (%s.fnt)?", path);
            break;
        }

        if (glyph->char_code >= 32 && glyph->char_code < 128)
            font->printable_ascii_lookup[glyph->char_code-32] = font->n_glyphs-1;

        r = true;
    }
    fclose(fp);
    if (false == r)
        return false;

    strncpy(buf, path, BUFLEN);
    strncat(buf, ".png", BUFLEN);
    return texture_from_png(&font->texture, buf);
}

void text_destroy_font(struct font *font)
{
    free(font->glyphs);
    memset(font, 0, sizeof (*font));
}

static struct glyph *lookup_glyph(struct font *font, const uint8_t **s)
{
    if (**s < 32) return NULL;
    int n = 0;
    if (**s < 128) {
        n = font->printable_ascii_lookup[**s - 32];
        if (n < 0) return NULL;
        return &font->glyphs[n];
    }

    // XXX decode UTF-8
    // XXX binary search
    return NULL;
}


// render text
//   look up glyph
//   render glyph and advance
//   - render one glyph at a time or queue up a bunch of points and texture coordinates and render them all at once?
//     start by rendering one glyph at a time
// simple shader
static void render_glyph(struct glyph *g, position p)
{
    // adjust camera
    // pass in texture coordinates
}

// setup font shader if it's not already setup
// XXX should font rendering be a singleton?  should there be a shader
// singleton that lives elsewhere that takes care of this for us?
static void setup_shader(struct font *font, uint32_t color)
{
    // XXX should assert thread identity
    // load the shader
    // use program
    // setup uniforms
}



/* # Options: */
/* #  - multiply each pixel by a uniform color */
/* #    - set this to white to preserve multicolor fonts */
/* #  - load alpha-only and full-color fonts */

// XXX what are the units of position?
void text_render_line(struct font *font, position p, uint32_t color, const uint8_t *s)
{
    ENSURE(s && font);

    setup_shader(font, color);

    for (; *s; ++s) {
        struct glyph *g = lookup_glyph(font, &s);
        if (!p) continue;       /* no glyph; just proceed.  In the future we could render a box. */
        render_glyph(g, p + (g->x_offset + g->y_offset * I));
        p += g->x_advance;
    }
}

void text_render_line_with_shadow(struct font *font, position p, uint32_t color, const uint8_t *s)
{
#define TEXT_SHADOW_OFFSET (5.f + 5.f*I)
    enum { TEXT_SHADOW_COLOR = 0xff };
    text_render_line(font, p+TEXT_SHADOW_OFFSET, TEXT_SHADOW_COLOR, s);
    text_render_line(font, p, color, s);
}
