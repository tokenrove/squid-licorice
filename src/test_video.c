/* An implementation of the video.c interface that uses MESA's
 * software rasterization
 */

#include <stdio.h>
#include <stdlib.h>
#include "gl.h"
#include <GL/osmesa.h>
#include <pnglite.h>

#include "video.h"
#include "test_video.h"
#include "ensure.h"
#include "libtap/tap.h"

uint16_t viewport_w, viewport_h;
static OSMesaContext ctx;
static uint8_t *framebuf;
static int bpp = 4;

static int bpp_to_color_type(int v)
{
    if (v == 3) return PNG_TRUECOLOR;
    if (v == 4) return PNG_TRUECOLOR_ALPHA;
    ENSURE(false);
}

static int color_type_to_bpp(int ct)
{
    if (ct == PNG_TRUECOLOR) return 3;
    if (ct == PNG_TRUECOLOR_ALPHA) return 4;
    ENSURE(false);
}

static void save_differing_screenshot(const char *path)
{
    char buf[128];
    strncpy(buf, path, 128);
    strncat(buf, ".test-output", 128-strlen(buf));
    video_save_fb_screenshot(buf);
    note("Differing output saved to %s", buf);
}

bool test_video_compare_fb_with_file(const char *path)
{
    png_t png;
    if (PNG_NO_ERROR != png_open_file_read(&png, path)) {
        save_differing_screenshot(path);
        return false;
    }
    int ct = bpp_to_color_type(bpp);
    if (!ok(png.width == viewport_w &&
            png.height == viewport_h &&
            png.color_type == ct,
            "Visual parameters (%dx%dx%d) should match saved screenshot (%dx%dx%d)",
            viewport_w, viewport_h, bpp,
            png.width, png.height, color_type_to_bpp(png.color_type))) {
        png_close_file(&png);
        return false;
    }
    size_t n = viewport_w * viewport_h;
    uint8_t *pels = malloc(n*bpp);
    ENSURE(pels);
    ENSURE(PNG_NO_ERROR == png_get_data(&png, pels));
    bool result = cmp_mem(framebuf, pels, n*bpp,
                          "Saved screenshot matches framebuffer");
    free(pels);
    png_close_file(&png);

    if (!result)
        save_differing_screenshot(path);
    return result;
}

void video_save_fb_screenshot(const char *path)
{
    png_t png;
    ENSURE(PNG_NO_ERROR == png_open_file_write(&png, path));
    int color_type = bpp_to_color_type(bpp);
    ENSURE(PNG_NO_ERROR == png_set_data(&png, viewport_w, viewport_h, 8, color_type, framebuf));
    ENSURE(PNG_NO_ERROR == png_close_file(&png));
}

static void video_atexit(void)
{
    OSMesaDestroyContext(ctx);
    free(framebuf);
    ctx = NULL;
    framebuf = NULL;
}

void video_init(void)
{
    if (viewport_w == 0) {
        viewport_w = 640;
        viewport_h = 480;
    }

    if (ctx != NULL) return;
    ENSURE(framebuf == NULL && ctx == NULL);
    GLenum format = (bpp == 3) ? OSMESA_RGB : OSMESA_RGBA;
    ENSURE(ctx = OSMesaCreateContext(format, NULL));
    ENSURE(framebuf = malloc(viewport_w * viewport_h * bpp));
    ENSURE(OSMesaMakeCurrent(ctx, framebuf, GL_UNSIGNED_BYTE, viewport_w, viewport_h));
    atexit(video_atexit);
    OSMesaPixelStore(OSMESA_Y_UP, 0);

    gl_init();
    png_init(NULL, NULL);
}

void video_start_frame(void) {
    glClear(GL_COLOR_BUFFER_BIT);
}

void video_end_frame(void) {
    glFinish();
}

