/* An implementation of the video.c interface that uses MESA's
 * software rasterization
 */

#include <stdlib.h>
#include "gl.h"
#include <GL/osmesa.h>

#include "video.h"
#include "ensure.h"

uint16_t viewport_w, viewport_h;
static OSMesaContext ctx;
static uint8_t *framebuf;
static int bpp = 4;

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

    ENSURE(framebuf == NULL && ctx == NULL);
    ENSURE(ctx = OSMesaCreateContext(OSMESA_RGBA, NULL));
    ENSURE(framebuf = malloc(viewport_w * viewport_h * bpp));
    ENSURE(OSMesaMakeCurrent(ctx, framebuf, GL_UNSIGNED_BYTE, viewport_w, viewport_h));
    atexit(video_atexit);

    gl_init();
}

void video_start_frame(void) { }

void video_end_frame(void) {
    glFinish();
}

