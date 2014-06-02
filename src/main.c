#include <SDL2/SDL.h>

#include "ensure.h"
#include "video.h"
#include "tilemap.h"
#include "text.h"
#include "camera.h"

int main(/* int argc, char **argv */)
{
    video_init();

    tilemap_init();
    struct tilemap t;
    ENSURE(tilemap_load("map", "atlas.png", &t));

    text_init();
    struct font font;
    ENSURE(text_load_font(&font, "myfont"));

    while (1) {
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
                return EXIT_SUCCESS;
            default: /* ignore */ break;
            }
        }
    }

    text_destroy_font(&font);
    tilemap_destroy(&t);
    return 0;
}
