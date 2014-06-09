
#include <math.h>
#include "camera.h"
#include "game.h"
#include "input.h"
#include "level.h"
#include "strand.h"
#include "text.h"
#include "tilemap.h"
#include "ensure.h"
#include "stage.h"
#include "main.h"
#include "video.h"
#include "physics.h"
#include "actor.h"
#include "log.h"

const int INITIAL_N_LIVES = 3;

struct game {
    int lives, level;
};

enum outcome { NO_OUTCOME = 0, OUTCOME_QUIT, OUTCOME_OUT_OF_LIVES, OUTCOME_NEXT_LEVEL };

static struct font font;

enum { MAX_N_BODIES = 128, MAX_N_ACTORS = 64 };

static enum outcome inner_game_loop(strand self, struct game *game)
{
    bodies_init(MAX_N_BODIES);
    actors_init(MAX_N_ACTORS);
    struct level *level = level_load(game->level);
    ENSURE(level);

    enum outcome outcome = NO_OUTCOME;
    float elapsed_time = 0.f;
    double accumulated_time = 0., last_fps_update = -1.;
    char fps_output[6] = {0};
    complex float fps_output_pos;

    // determine font metrics for placement of ready, fps messages
    fps_output_pos = (viewport_w - 60.f)  + (viewport_h - 30.f)*I;

    do {
        stage_draw();
        actors_draw();

        // osd
        if (accumulated_time < 5. && fmod(accumulated_time, 1.) <= .5) {
            // XXX use an interpolation curve here
            float alpha = (5. - accumulated_time)/5.;
            text_render_line(&font, viewport_w/2.f + viewport_h/2.f*I, 0xff000000 | (int)(0xff*alpha), "READY");
            //text_render_line(&font, 1024/2.f + 768/2.f*I, 0xff0000ff, level->name);
        }
        // fps meter
        if (accumulated_time - last_fps_update >= 1.) {
            snprintf(fps_output, sizeof (fps_output), "%.1f", 1./average_frame_time);
            last_fps_update = accumulated_time;
        }
        text_render_line(&font, fps_output_pos, 0xff000080, fps_output);

        elapsed_time = strand_yield(self);
        accumulated_time += (double)elapsed_time;

        stage_update(elapsed_time);
        strand_resume(level->strand, elapsed_time);
        actors_update(elapsed_time);

        if (inputs[IN_MENU]) world_camera.scaling += 0.01f;
        if (inputs[IN_QUIT])
            outcome = OUTCOME_QUIT;

        if (!strand_is_alive(level->strand))
            outcome = OUTCOME_NEXT_LEVEL;
    } while (NO_OUTCOME == outcome);

    level_destroy(level);
    actors_destroy();
    bodies_destroy();

    return outcome;
}

static void outer_game_loop(strand self, struct game *game)
{
    while(true) {
        enum outcome outcome = inner_game_loop(self, game);

        switch(outcome) {
        case OUTCOME_QUIT: return;
        case OUTCOME_OUT_OF_LIVES:
            // if (continue_screen(self)) { game->lives = INITIAL_N_LIVES; continue; }
            // game_over_screen();
            return;
        case OUTCOME_NEXT_LEVEL:
            ++game->level;
            if (game->level >= N_LEVELS) {
                // ending_screen();
                return;
            }
            break;
        default:
            LOG_DEBUG("Bad outcome: %d", outcome);
            break;
        }
    }
}

static void new_game(strand self)
{
    struct game game = {.lives = INITIAL_N_LIVES, .level = 0};
    outer_game_loop(self, &game);
}

static void main_menu_loop(strand self)
{
    ENSURE(text_load_font(&font, "myfont"));
    new_game(self);
    text_destroy_font(&font);
}

void game_entry_point(strand self)
{
    camera_init();
    tilemap_init();
    text_init();
    sprite_init();
    main_menu_loop(self);
}
