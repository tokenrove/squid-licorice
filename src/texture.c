
#include <stdbool.h>

#include <pnglite.h>

#include <GL/gl.h>

#include "ensure.h"
#include "texture.h"
#include "log.h"

void texture_init(void)
{
    static bool initp = false;
    if (initp) return;
    initp = true;
    png_init(NULL, NULL);
}

static void reset_pixel_store(void)
{
    // reset pixel store: can we use a pack/unpack alignment of 4 here?
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
}

bool texture_from_pels(struct texture *t, uint8_t *pels,
                       uint16_t w, uint16_t h, uint8_t bpp)
{
    *t = (struct texture){0};
    t->width = w;
    t->height = h;
    glGenTextures(1, &t->id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t->id);
    reset_pixel_store();
    GLint internal_format;
    GLenum format;
    switch(bpp) {
    case 3:
        internal_format = GL_BGR;
        format = GL_RGB;
        break;
    case 4:
        internal_format = GL_BGRA;
        format = GL_RGBA;
        break;
    default:
        internal_format = GL_R8;
        format = GL_RED;
        break;
    };
    // XXX needs to be power of 2? what about alignment by 4s?
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, format, GL_UNSIGNED_BYTE, pels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return true;
}


bool texture_from_png(struct texture *t, const char *path)
{
    *t = (struct texture){0};

    png_t png;
    int outcome = png_open_file(&png, path);
    if (PNG_NO_ERROR != outcome) {
        LOG_ERROR("png_open_file(%s): %d", path, outcome);
        return false;
    }
    uint8_t *pels;
    pels = malloc(png.width*png.height*png.bpp);
    ENSURE(pels);
    ENSURE(PNG_NO_ERROR == png_get_data(&png, pels));

    t->width = png.width;
    t->height = png.height;

    glGenTextures(1, &t->id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t->id);
    GLuint mode = (png.color_type == PNG_TRUECOLOR_ALPHA) ? GL_RGBA : GL_RGB;
    ENSURE(png.color_type == PNG_TRUECOLOR_ALPHA ||
           png.color_type == PNG_TRUECOLOR);
    reset_pixel_store();
    glTexImage2D(GL_TEXTURE_2D, 0, mode, png.width, png.height, 0, mode, GL_UNSIGNED_BYTE, pels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    png_close_file(&png);
    free(pels);

    return true;
}

void texture_destroy(struct texture *t)
{
    glDeleteTextures(1, &t->id);
}

#ifdef UNIT_TEST_TEXTURE
#include "libtap/tap.h"
#include "video.h"

static void basic_check_texture(const char *path, int w, int h)
{
    struct texture t;
    note("checking %s", path);
    if (!ok(texture_from_png(&t, path))) return;
    cmp_ok(t.width, "==", w);
    cmp_ok(t.height, "==", h);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, t.id);
    GLint i = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &i);
    cmp_ok(i, "==", t.width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &i);
    cmp_ok(i, "==", t.height);
    texture_destroy(&t);
}

static void test_png_loading(void)
{
    struct texture t;
    ok(!texture_from_png(&t, "t/texture.t.from-png.non-existent.png"));

    basic_check_texture("t/texture.t.from-png.rgba.png", 64, 32);
    basic_check_texture("t/texture.t.from-png.rgb.png", 64, 32);
    todo();
    basic_check_texture("t/texture.t.from-png.indexed.png", 64, 32);
    end_todo;
    basic_check_texture("t/texture.t.from-png.npot.png", 65, 48);
}

int main(void)
{
    video_init();
    texture_init();
    plan(20);
    test_png_loading();
    todo();
    pass("test reading from raw data");
    pass("test texture cache");
    pass("test destroying textures");
    end_todo;
    done_testing();
}
#endif
