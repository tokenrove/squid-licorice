#include <SDL2/SDL.h>

#include "ensure.h"
#include "video.h"
#include "timer.h"
#include "tilemap.h"
#include "text.h"
#include "camera.h"

enum outcome { NO_OUTCOME = 0, OUTCOME_QUIT };

static inline float max(const float a, const float b) { return a < b ? b : a; }

static float update_frame_timer(void)
{
    const float target_frame_time = 1./60.;
    static uint32_t last_ticks = 0;
    uint32_t now = timer_ticks_ms();
    float elapsed_time = (now - last_ticks) / 1000.;
    last_ticks = now;
    /* The first time through this routine, or if the machine is
     * performing egregiously poorly, elapsed_time will be very large,
     * so we'll pretend it was a normal frame. */
    if (elapsed_time > 3.f*target_frame_time)
        elapsed_time = target_frame_time;
    /* After a timer overflow, elapsed_time will be negative, and so
     * we'll sleep to around the target frame time.  This is also
     * avoids very small time deltas, in the case of a very fast
     * machine (or logic error somewhere), which could cause numerical
     * instability elsewhere. */
    if (elapsed_time < target_frame_time/4.f) {
        timer_sleep_ms(target_frame_time - max(0., elapsed_time));
        elapsed_time = target_frame_time;
    }
    return elapsed_time;
}

int main(/* int argc, char **argv */)
{
    video_init();

    tilemap_init();
    struct tilemap t;
    ENSURE(tilemap_load("map", "atlas.png", &t));

    text_init();
    struct font font;
    ENSURE(text_load_font(&font, "myfont"));

    enum outcome outcome = NO_OUTCOME;
    do {
        float elapsed_time = update_frame_timer();

        video_start_frame();
        tilemap_draw(&t, camera_world_mv_matrix);
        text_render_line(&font, 50.f + 50.f*I, 0xff0000ff, "This is a test!");
        text_render_line_with_shadow(&font, 50.f + 80.f*I, 0x00ff00ff, "Hello, world.");
        text_render_line_with_shadow(&font, 50.f + 120.f*I, 0xffffff80, "Hello — Montréal ©");
        video_end_frame();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_KEYDOWN:
                if (e.key.state != SDL_PRESSED) break;
                switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    SDL_PushEvent((SDL_Event*)&(SDL_QuitEvent){.type=SDL_QUIT});
                    break;
                case SDLK_DOWN:
                    camera_world_mv_matrix[13] += 2.f;
                    break;
                case SDLK_UP:
                    camera_world_mv_matrix[13] -= 2.f;
                    break;
                case SDLK_LEFT:
                    camera_world_mv_matrix[12] -= 2.f;
                    break;
                case SDLK_RIGHT:
                    camera_world_mv_matrix[12] += 2.f;
                    break;
                default: break;
                };
                break;
            case SDL_QUIT:
                outcome = OUTCOME_QUIT;
                break;
            default: /* ignore */ break;
            }
        }
    } while(NO_OUTCOME == outcome);

    text_destroy_font(&font);
    tilemap_destroy(&t);
    return 0;
}
