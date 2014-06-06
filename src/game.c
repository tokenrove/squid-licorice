
#include "camera.h"
#include "game.h"
#include "input.h"
#include "stage.h"
#include "strand.h"
#include "text.h"
#include "tilemap.h"
#include "ensure.h"

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

    struct tilemap t;
    ENSURE(tilemap_load("map", "atlas.png", &t));

    do {
        tilemap_draw(&t, camera_world_mv_matrix);
        text_render_line(&font, 50.f + 50.f*I, 0xff0000ff, "This is a test!");
        text_render_line_with_shadow(&font, 50.f + 80.f*I, 0x00ff00ff, "Hello, world.");
        text_render_line_with_shadow(&font, 50.f + 120.f*I, 0xffffff80, "Hello — Montréal ©");

        elapsed_time = strand_yield(self);

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
