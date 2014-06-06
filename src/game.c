
#include <math.h>
#include "camera.h"
#include "game.h"
#include "input.h"
#include "stage.h"
#include "strand.h"
#include "text.h"
#include "tilemap.h"
#include "ensure.h"
#include "main.h"

const int INITIAL_N_LIVES = 3;

struct game {
    int lives, stage;
};

enum outcome { NO_OUTCOME = 0, OUTCOME_QUIT, OUTCOME_OUT_OF_LIVES, OUTCOME_NEXT_LEVEL };

static struct font font;

static enum outcome inner_game_loop(strand self, struct game *game, struct stage *stage)
{
    enum outcome outcome = NO_OUTCOME;
    float elapsed_time = 0.;
    double accumulated_time = 0., last_fps_update = -1.;
    char fps_output[6] = {0};
    complex float fps_output_pos;

    struct tilemap t;
    ENSURE(tilemap_load("map", "atlas.png", &t));

    // determine font metrics for placement of ready, fps messages
    fps_output_pos = (1024.f - 60.f)  + (768.f - 30.f)*I;

    do {
        tilemap_draw(&t, camera_world_mv_matrix);

        // osd
        if (accumulated_time < 5. && fmod(accumulated_time, 1.) <= .5) {
            text_render_line(&font, 1024/2.f + 768/2.f*I, 0xff0000ff, "READY");
            //text_render_line(&font, 1024/2.f + 768/2.f*I, 0xff0000ff, stage->name);
        }
        // fps meter
        if (accumulated_time - last_fps_update >= 1.) {
            snprintf(fps_output, sizeof (fps_output), "%.1f", 1./average_frame_time);
            last_fps_update = accumulated_time;
        }
        text_render_line(&font, fps_output_pos, 0xff0000ff, fps_output);

        elapsed_time = strand_yield(self);
        accumulated_time += (double)elapsed_time;

        strand_resume(stage->strand, elapsed_time);

        if (inputs[IN_UP])
            camera_world_mv_matrix[13] -= 2.f;
        if (inputs[IN_DOWN])
            camera_world_mv_matrix[13] += 2.f;
        if (inputs[IN_LEFT])
            camera_world_mv_matrix[12] -= 2.f;
        if (inputs[IN_RIGHT])
            camera_world_mv_matrix[12] += 2.f;
        if (inputs[IN_QUIT])
            outcome = OUTCOME_QUIT;
        if (inputs[IN_MENU] == JUST_PRESSED)
            SDL_Log("Just hit menu");
        if (inputs[IN_SHOOT] == JUST_PRESSED)
            SDL_Log("Just hit shoot");

        if (!strand_is_alive(stage->strand))
            outcome = OUTCOME_NEXT_LEVEL;
    } while (NO_OUTCOME == outcome);

    tilemap_destroy(&t);
    return outcome;
}

static void outer_game_loop(strand self, struct game *game)
{
    while(true) {
        struct stage *stage = stage_load(game->stage);
        ENSURE(stage);

        enum outcome outcome = inner_game_loop(self, game, stage);

        stage_destroy(stage);

        switch(outcome) {
        case OUTCOME_QUIT: return;
        case OUTCOME_OUT_OF_LIVES:
            // if (continue_screen(self)) { game->lives = INITIAL_N_LIVES; continue; }
            // game_over_screen();
            return;
        case OUTCOME_NEXT_LEVEL:
            ++game->stage;
            if (game->stage >= N_STAGES) {
                // ending_screen();
                return;
            }
            break;
        default:
            SDL_Log("Bad outcome: %d", outcome);
            break;
        }
    }
}

static void new_game(strand self)
{
    struct game game = {.lives = INITIAL_N_LIVES, .stage = 0};
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
    tilemap_init();
    text_init();
    main_menu_loop(self);
}
