
#include "video.h"
#include "easing.h"
#include <math.h>
#include "osd.h"
#include "main.h"
#include "text.h"
#include "ensure.h"

static struct font font;
static double accumulated_time = 0.;
static char fps_output[6] = {0};


void osd_init(void)
{
    ENSURE(text_load_font(&font, "myfont"));
    accumulated_time = 0.;
}

void osd_destroy(void)
{
    text_destroy_font(&font);
}

static void draw_messages(void)
{
    if (accumulated_time < 5. && fmod(accumulated_time, 1.) <= .5) {
        float alpha = 1. - easing_cubic(accumulated_time, 5.);
        text_render_line_with_shadow(&font, viewport_w/2.f + viewport_h/2.f*I, 0xffffff00 | (int)(0xff*alpha), "READY");
    }
}

void osd_draw(void)
{
    draw_messages();
    // determine font metrics for placement of ready, fps messages
    complex float fps_output_pos = (viewport_w - 60.f)  + (viewport_h - 30.f)*I;
    text_render_line(&font, fps_output_pos, 0xff000080, fps_output);
}

void osd_update(float elapsed_time)
{
    static double last_fps_update = -1.;

    accumulated_time += (double)elapsed_time;
    if (accumulated_time - last_fps_update >= 1.) {
        snprintf(fps_output, sizeof (fps_output), "%.1f", 1./average_frame_time);
        last_fps_update = accumulated_time;
    }
}
