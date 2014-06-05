
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
#include <GL/glew.h>
#include <GL/gl.h>

#include "text.h"
#include "ensure.h"
#include "shader.h"
#include "camera.h"

#define LOG(message, ...) do {                 \
        fprintf(stderr, "%s: ", __func__);     \
        fprintf(stderr, message, __VA_ARGS__); \
        fputc('\n', stderr);                   \
    } while(0)


static const GLchar text_vertex_shader_src[] = {
#include "text.vert.i"
    , 0 };
static const GLchar text_fragment_shader_src[] = {
#include "text.frag.i"
    , 0 };
static GLuint shader;

void text_init(void)
{
    texture_init();
    if (0 == shader)
        shader = build_shader_program("text", text_vertex_shader_src, text_fragment_shader_src);
    ENSURE(shader);
}

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
        // XXX note that these font files by default leave the
        // character code UTF-8 encoded, stupidly, but I've mangled
        // them ahead of time.
        if (8 != sscanf(buf, "%u %hu %hu %hhu %hhu %hhd %hhd %hhd",
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

    glGenBuffers(1, &font->a_vertices);

    strncpy(buf, path, BUFLEN);
    strncat(buf, ".png", BUFLEN);
    return texture_from_png(&font->texture, buf);
}

void text_destroy_font(struct font *font)
{
    free(font->glyphs);
    glDeleteBuffers(1, &font->a_vertices);
    memset(font, 0, sizeof (*font));
}

/* Very primitive UTF-8 decoding. */
static uint32_t decode_utf8(const char **s)
{
    uint8_t c = **s;
    if (c < 128) return c;
    c <<= 1;
    ENSURE(c&0x80);
    int n;
    for (n = 0; c&0x80; ++n) c <<= 1;
    uint32_t u = c>>(1+n);
    while (n--) {
        c = *(++*s);
        ENSURE(0x80 == (c&0xc0));
        u = (u<<6) | (c&0x3f);
    }
    return u;
}

static struct glyph *lookup_glyph(struct font *font, const char **s)
{
    uint32_t u = decode_utf8(s);
    if (u < 32) return NULL;
    if (u < 128) {
        int n = font->printable_ascii_lookup[u - 32];
        if (n < 0) return NULL;
        return &font->glyphs[n];
    }
    for (int i = 0; i < font->n_glyphs; ++i)
        if (font->glyphs[i].char_code == u)
            return &font->glyphs[i];
    // XXX binary search
    return NULL;
}

static void render_glyph(struct glyph *g, position p)
{
    // adjust camera
    float mv[16];
    memcpy(mv, camera_screen_mv_matrix, sizeof (mv));
    mv[12] = crealf(p);
    mv[13] = cimagf(p);

    glUniformMatrix4fv(glGetUniformLocation(shader, "u_modelview"),
                       1, GL_FALSE, mv);
    glUniform2f(glGetUniformLocation(shader, "u_offset"), g->x, g->y);
    const GLfloat vertices[] = { 0., 0., g->w, 0., g->w, g->h, 0., g->h };
    glBufferData(GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_DYNAMIC_DRAW);

    uint8_t indices[] = { 0,1,2, 2,3,0 };
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
}

// setup font shader if it's not already setup
// XXX should font rendering be a singleton?  should there be a shader
// singleton that lives elsewhere that takes care of this for us?
static void setup_shader(struct font *font, uint32_t color)
{
    // XXX should assert thread identity
    ENSURE(shader);

    glUseProgram(shader);

    glBindBuffer(GL_ARRAY_BUFFER, font->a_vertices);
    GLint vertices_atloc = glGetAttribLocation(shader, "a_vertex");
    glVertexAttribPointer(vertices_atloc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertices_atloc);
    glEnableClientState(GL_VERTEX_ARRAY);

    glUniformMatrix4fv(glGetUniformLocation(shader, "u_modelview"),
                       1, GL_FALSE, camera_screen_mv_matrix);
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_projection"),
                       1, GL_FALSE, camera_projection_matrix);

    glUniform4f(glGetUniformLocation(shader, "u_color"),
                (color>>24&0xff)/255.,
                (color>>16&0xff)/255.,
                (color>>8&0xff)/255.,
                (color&0xff)/255.);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->texture.id);
    glUniform1i(glGetUniformLocation(shader, "u_font"), 0);
    glUniform2f(glGetUniformLocation(shader, "u_font_dims"),
                font->texture.width, font->texture.height);
}



/* # Options: */
/* #  - multiply each pixel by a uniform color */
/* #    - set this to white to preserve multicolor fonts */
/* #  - load alpha-only and full-color fonts */

// XXX what are the units of position?
void text_render_line(struct font *font, position p, uint32_t color, const char *s)
{
    ENSURE(s && font);

    setup_shader(font, color);

    for (; *s; ++s) {
        struct glyph *g = lookup_glyph(font, &s);
        if (!g) continue;       /* no glyph; just proceed.  In the future we could render a box. */
        render_glyph(g, p + (g->x_offset + g->y_offset * I));
        p += g->x_advance;
    }
}

void text_render_line_with_shadow(struct font *font, position p, uint32_t color, const char *s)
{
#define TEXT_SHADOW_OFFSET (5.f + 5.f*I)
    enum { TEXT_SHADOW_COLOR = 0xff };
    text_render_line(font, p+TEXT_SHADOW_OFFSET, TEXT_SHADOW_COLOR, s);
    text_render_line(font, p, color, s);
}