
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
#include "log.h"

static const GLchar text_vertex_shader_src[] = {
#include "text.vert.i"
    , 0 };
static const GLchar text_fragment_shader_src[] = {
#include "text.frag.i"
    , 0 };
static GLuint shader, a_vertices;

void text_init(void)
{
    texture_init();
    if (0 == shader)
        shader = build_shader_program("text", text_vertex_shader_src, text_fragment_shader_src);
    ENSURE(shader);
    glGenBuffers(1, &a_vertices);
}

static bool read_glyph(struct font *font, const char *buf, const char *path)
{
    int8_t y_adv;
    struct glyph *glyph;

    ++font->n_glyphs;
    // XXX next font format should declare the number of glyphs up
    // front, and force them to occur in sorted order.
    font->glyphs = realloc(font->glyphs, font->n_glyphs * sizeof (*font->glyphs));
    if (NULL == font->glyphs) {
        LOG_ERROR("Failed to allocate memory for glyphs (%s.fnt)", path);
        return false;
    }

    if ('\n' != buf[strlen(buf)-1]) {
        LOG_DEBUG("Sorry, we expect every font file line to end in an LF (%s.fnt)", path);
        return false;
    }
    // Also y_advance at the end of the line, but we ignore it.
    glyph = &font->glyphs[font->n_glyphs-1];
    // XXX note that these font files by default leave the
    // character code UTF-8 encoded, stupidly, but I've mangled
    // them ahead of time.
    if (9 != sscanf(buf, "%u %hu %hu %hhu %hhu %hhd %hhd %hhd %hhd",
                    &glyph->char_code, &glyph->x, &glyph->y,
                    &glyph->w, &glyph->h, &glyph->x_offset, &glyph->y_offset,
                    &glyph->x_advance, &y_adv)) {
        LOG_DEBUG("Didn't get exactly nine values; misformatted font file (%s.fnt)?", path);
        return false;
    }

    if (y_adv > font->line_height)
        font->line_height = y_adv;

    if (glyph->char_code >= 32 && glyph->char_code < 128)
        font->printable_ascii_lookup[glyph->char_code-32] = font->n_glyphs-1;

    return true;
}

/*
 * path: NUL-terminated string containing the path to the font file
 * without extension (foo passed in loads foo.fnt and foo.png)
 */
bool text_load_font(struct font *font, const char *path)
{
    enum { BUFLEN = 127 };
    char buf[BUFLEN+1] = {0};
    FILE *fp;

    *font = (struct font){0};

    strncpy(buf, path, BUFLEN);
    strncat(buf, ".fnt", BUFLEN-strlen(buf));
    fp = fopen(buf, "rb");
    if (NULL == fp) {
        LOG_ERROR("file not found (%s)", buf);
        return false;
    }
    bool r = false;
    fgets(buf, BUFLEN, fp);  // skip first line
    do {
        if (NULL == fgets(buf, BUFLEN, fp)) break;
    } while((r = read_glyph(font, buf, path)) && !feof(fp));
    fclose(fp);
    if (false == r)
        return false;

    strncpy(buf, path, BUFLEN);
    strncat(buf, ".png", BUFLEN-strlen(buf));
    return texture_from_png(&font->texture, buf);
}

void text_destroy_font(struct font *font)
{
    free(font->glyphs);
    glDeleteBuffers(1, &a_vertices);
    memset(font, 0, sizeof (*font));
}

/* Very primitive UTF-8 decoding. */
static uint32_t decode_utf8(const char **s)
{
    uint8_t c = **s;
    ++*s;
    if (c < 128)
        return c;
    c <<= 1;
    if(!(c&0x80)) goto invalid;
    int n;
    for (n = 0; c&0x80; ++n) c <<= 1;
    uint32_t u = c>>(1+n);
    while (n--) {
        c = **s;
        if (!c) goto invalid;
        ++*s;
        if (0x80 != (c&0xc0)) goto invalid;
        u = (u<<6) | (c&0x3f);
    }
    return u;
invalid:
    LOG_DEBUG("Encountered invalid UTF-8 encoded string; discarding.");
    return 0;
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
    p += osd_camera.translation;
    glUniform2f(glGetUniformLocation(shader, "u_translation"),
                crealf(p),
                cimagf(p));
    glUniform2f(glGetUniformLocation(shader, "u_offset"), g->x, g->y);
    const GLfloat vertices[] = { 0., 0., g->w, 0., g->w, g->h, 0., g->h };
    glBufferData(GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_DYNAMIC_DRAW);

    static const uint8_t indices[] = { 0,1,2, 2,3,0 };
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
}

static void setup_shader(struct font *font, uint32_t color)
{
    // XXX should assert thread identity
    ENSURE(shader);

    glUseProgram(shader);

    glBindBuffer(GL_ARRAY_BUFFER, a_vertices);
    GLint vertices_atloc = glGetAttribLocation(shader, "a_vertex");
    glVertexAttribPointer(vertices_atloc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertices_atloc);
    glEnableClientState(GL_VERTEX_ARRAY);

    glUniform2f(glGetUniformLocation(shader, "u_translation"),
                crealf(osd_camera.translation),
                cimagf(osd_camera.translation));
    glUniform1f(glGetUniformLocation(shader, "u_scaling"),
                osd_camera.scaling);
    glUniform1f(glGetUniformLocation(shader, "u_rotation"),
                osd_camera.rotation);
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_projection"),
                       1, GL_FALSE, camera_projection_matrix);

    glUniform4f(glGetUniformLocation(shader, "u_color"),
                ((color>>24)&0xff)/255.,
                ((color>>16)&0xff)/255.,
                ((color>>8)&0xff)/255.,
                (color&0xff)/255.);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->texture.id);
    glUniform1i(glGetUniformLocation(shader, "u_font"), 0);
    glUniform2f(glGetUniformLocation(shader, "u_font_dims"),
                font->texture.width, font->texture.height);
}

void text_render_line(struct font *font, position p, uint32_t color, const char *s)
{
    ENSURE(s && font);

    setup_shader(font, color);

    while (*s) {
        struct glyph *g = lookup_glyph(font, &s);
        if (!g) continue; /* no glyph; just proceed.  In the future we could render a box. */
        render_glyph(g, p + (g->x_offset + g->y_offset * I));
        p += g->x_advance;
    }
}

void text_render_line_with_shadow(struct font *font, position p, uint32_t color, const char *s)
{
#define TEXT_SHADOW_OFFSET (5.f + 5.f*I)
    enum { TEXT_SHADOW_COLOR = 0x80 };
    text_render_line(font, p+TEXT_SHADOW_OFFSET, TEXT_SHADOW_COLOR, s);
    text_render_line(font, p, color, s);
}

uint16_t text_get_line_height(struct font *font)
{
    return font->line_height;
}

uint16_t text_width(struct font *font, const char *s)
{
    if (!font || !s) return 0;
    uint16_t w = 0;
    while (*s) {
        struct glyph *g = lookup_glyph(font, &s);
        if (!g) continue; /* no glyph; just proceed.  In the future we could render a box. */
        w += g->x_advance;
    }
    return w;
}

#ifdef UNIT_TEST_TEXT
#include "libtap/tap.h"
#include "video.h"
#include "test_video.h"

static void test_font_file_reading(void)
{
    struct font font;
    note("font file reading");
    ok(!text_load_font(&font, "t/text.t.does-not-exist"), "Non-existent file");
    ok(text_load_font(&font, "t/text.t.0"), "Load a valid font");
    cmp_ok(font.n_glyphs, "==", 94);
    text_destroy_font(&font);
    ok(!text_load_font(&font, "t/text.t.1-missing-png"), "Font with missing PNG");
    ok(!text_load_font(&font, "t/text.t.2-empty"), "Empty file");
    ok(!text_load_font(&font, "t/text.t.3-empty"), "Empty file with blank line");
    ok(!text_load_font(&font, "t/text.t.4-truncated"), "Truncated file");
    todo();
    ok(text_load_font(&font, "t/text.t.5-extreme-values"), "Extreme values");
    cmp_ok(font.n_glyphs, "==", 1);
    text_destroy_font(&font);
    end_todo;
    ok(text_load_font(&font, "t/text.t.6-crlf-endings"), "CRLF endings");
    cmp_ok(font.n_glyphs, "==", 94);
    text_destroy_font(&font);
    todo();
    ok(!text_load_font(&font, "t/text.t.7-duplicate-glyphs"), "Duplicate entries");
    end_todo;
}

static void test_shader_output(void)
{
    struct font font;
    note("Testing basic shader output");
    ok(text_load_font(&font, "t/text.t.0"));
    video_start_frame();
    position p = 0;
    text_render_line(&font, p, 0xffffffff, "This is one line of text, without shadow.");
    p += I*text_get_line_height(&font);
    text_render_line_with_shadow(&font, p, 0xffffffff, "This is another line of text, with shadow.");
    p += I*text_get_line_height(&font);
    text_render_line_with_shadow(&font, p, 0x00ff00ff, "This is green text with shadow.");
    p += I*text_get_line_height(&font);
    text_render_line_with_shadow(&font, p, 0x0000ff80, "This is blue text with shadow and alpha.");
    position center = viewport_w/2  + (viewport_h/2)*I;
    text_render_line(&font, center, 0xff000080, "This is a positioned line of text");
    video_end_frame();
    if (!test_video_compare_fb_with_file("t/text.t.shader-0.640x480.png")) {
        note("Differing output saved to t/text.t.shader-0.640x480-diff.png");
        video_save_fb_screenshot("t/text.t.shader-0.640x480-diff.png");
    }
    text_destroy_font(&font);
}

static void test_text_width(void)
{
    struct font font;
    note("Testing text width");
    ok(text_load_font(&font, "t/text.t.0"));
    cmp_ok(text_width(&font, "This is a positioned line of text"), "==", 389);
    cmp_ok(text_width(&font, "\u201cMontréal, Québec\u201d"), "==", 251);
    cmp_ok(text_width(&font, ""), "==", 0);
    cmp_ok(text_width(&font, NULL), "==", 0);
    text_destroy_font(&font);
}

static void test_ordinary_utf8_handling(void)
{
    struct font font;
    ok(text_load_font(&font, "t/text.t.0"));
    video_start_frame();
    position p = 0;
    text_render_line(&font, p, 0xffffffff, "\u201cMontréal, Québec\u201d");
    p += I*text_get_line_height(&font);
    text_render_line_with_shadow(&font, p, 0x00ff00ff, "\u00a9 and \u00ae JS \u2014 2014");
    p += I*text_get_line_height(&font);
    text_render_line(&font, p, 0x0000ffff, "These characters are missing: \u2603 and \u2601");
    p += I*text_get_line_height(&font);
    video_end_frame();
    test_video_compare_fb_with_file("t/text.t.utf8-0.640x480.png");
    text_destroy_font(&font);
}

static void apply_fuzz_test_to(void (*fn)(void *, const char *), void *u)
{
    for (int i = 1000; i > 0; --i) {
        char buf[128];
        for (int j = 0; j < 128; ++j)
            buf[j] = (uint8_t)(rand() & 0xff);
        buf[127] = 0;
        fn(u, buf);
    }
}

static void test_low_level_utf8_decoding(void)
{
    note("Testing valid UTF-8");
    {
        const char *s = "\u201cMontréal, Québec\u201d";
        const uint32_t expected[] = {
            0x201c, 'M', 'o', 'n', 't', 'r', 0xe9, 'a', 'l', ',', ' ',
            'Q', 'u', 0xe9, 'b', 'e', 'c', 0x201d
        };
        for (const uint32_t *p = expected; *s; ++p) cmp_ok(decode_utf8(&s), "==", *p);
    }
    {
        note("Greek text");
        const char *s = "\316\272\341\275\271\317\203\316\274\316\265";
        const uint32_t expected[] = {  /* κόσμε */
            0x3ba,0x1f79,0x3c3,0x3bc,0x3b5
        };
        for (const uint32_t *p = expected; *s; ++p) cmp_ok(decode_utf8(&s), "==", *p);
    }
    {
        note("Hebrew text");
        /* ? דג סקרן שט בים מאוכזב ולפתע מצא לו חברה איך הקליטה */
        const char t[] = {
0x3f,0x20,0xd7,0x93,0xd7,0x92,0x20,0xd7,0xa1,0xd7,0xa7,0xd7,0xa8,0xd7,0x9f,0x20,
0xd7,0xa9,0xd7,0x98,0x20,0xd7,0x91,0xd7,0x99,0xd7,0x9d,0x20,0xd7,0x9e,0xd7,0x90,
0xd7,0x95,0xd7,0x9b,0xd7,0x96,0xd7,0x91,0x20,0xd7,0x95,0xd7,0x9c,0xd7,0xa4,0xd7,
0xaa,0xd7,0xa2,0x20,0xd7,0x9e,0xd7,0xa6,0xd7,0x90,0x20,0xd7,0x9c,0xd7,0x95,0x20,
0xd7,0x97,0xd7,0x91,0xd7,0xa8,0xd7,0x94,0x20,0xd7,0x90,0xd7,0x99,0xd7,0x9a,0x20,
0xd7,0x94,0xd7,0xa7,0xd7,0x9c,0xd7,0x99,0xd7,0x98,0xd7,0x94,0};
        const char *s = t;
        const uint32_t expected[] = {
63, 32, 1491, 1490, 32, 1505, 1511, 1512, 1503, 32, 1513, 1496, 32, 1489, 1497,
1501, 32, 1502, 1488, 1493, 1499, 1494, 1489, 32, 1493, 1500, 1508, 1514, 1506,
32, 1502, 1510, 1488, 32, 1500, 1493, 32, 1495, 1489, 1512, 1492, 32, 1488,
1497, 1498, 32, 1492, 1511, 1500, 1497, 1496, 1492
        };
        for (const uint32_t *p = expected; *s; ++p) cmp_ok(decode_utf8(&s), "==", *p);
    }

    note("Testing invalid UTF-8");
    {
        const char *t = "\xF0\xA4\xAD \xF0\xA4\xAD\xA2 \xF0\xA4\xAD\xA2 \xF0\xA4\xAD\xA2 \xF0\xA4\xAD\xA2 \xF0\xA4\xAD", *s = t;
        while (*s) {
            decode_utf8(&s);
        }
        cmp_ok((uint8_t)*(s-1), "==", 0xad, "s should be one past the last byte");
        cmp_ok((ptrdiff_t)(s-t), "==", strlen(t), "s (%p) should be right at %p", s, t+strlen(t));
    }

    note("Testing truncated UTF-8");
    {
        const char *t = "\xf0", *s = t;
        ok(0 == decode_utf8(&s));
        ok(0 == *s);
        cmp_ok((uint8_t)*(s-1), "==", 0xf0, "s should be one past the last byte");
        cmp_ok((intptr_t)s-1, "==", (intptr_t)t, "s (%p) should be one past the beginning (%p)", s, t);
    }

    lives_ok({
            log_quiet();
            void fn(void *_ __attribute__ ((unused)), const char *s) {
                while (*s) decode_utf8(&s);
            };
            apply_fuzz_test_to(fn, NULL);
            log_noisy();
        }, "Fuzz testing UTF-8 decoding");
}

static void test_unusual_utf8_handling(void)
{
    struct font font;
    ok(text_load_font(&font, "t/text.t.0"));
    video_start_frame();
    position p = 0;
    lives_ok({text_render_line(&font, p, 0x0000ffff, "\xF0\xA4\xAD \xF0\xA4\xAD\xA2 \xF0\xA4\xAD\xA2 \xF0\xA4\xAD\xA2 \xF0\xA4\xAD\xA2 \xF0\xA4\xAD");}, "Testing invalid UTF-8");
    lives_ok({text_render_line(&font, p, 0x0000ffff, "\xF0");}, "Testing truncated UTF-8");

    lives_ok({
        log_quiet();
        void fn(void *_ __attribute__ ((unused)), const char *s) {
            text_render_line(&font, p, rand(), s);
            p += I*((rand()&0xff) - 0x7f) + ((rand()&0xff) - 0x7f);
        };
        apply_fuzz_test_to(fn, NULL);
        log_noisy();
        }, "Fuzz testing");

    video_end_frame();
    text_destroy_font(&font);
}

int main(void)
{
    video_init();
    camera_init();
    text_init();
    plan(109);
    long seed = time(NULL);
    srand(seed);
    note("Random seed: %d", seed);
    test_font_file_reading();
    test_shader_output();
    test_text_width();
    test_ordinary_utf8_handling();
    test_low_level_utf8_decoding();
    test_unusual_utf8_handling();
    done_testing();
}
#endif
