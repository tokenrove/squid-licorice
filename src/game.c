
#include <math.h>

#include "actor.h"
#include "audio.h"
#include "camera.h"
#include "ensure.h"
#include "game.h"
#include "game_constants.h"
#include "input.h"
#include "level.h"
#include "log.h"
#include "main.h"
#include "msg.h"
#include "msg_macros.h"
#include "music.h"
#include "osd.h"
#include "physics.h"
#include "projectile.h"
#include "sfx.h"
#include "stage.h"
#include "strand.h"
#include "text.h"
#include "tilemap.h"
#include "video.h"

const int INITIAL_N_LIVES = 3;

struct game {
    int lives, level;
};

enum outcome { NO_OUTCOME = 0, OUTCOME_QUIT, OUTCOME_OUT_OF_LIVES, OUTCOME_NEXT_LEVEL };

enum { MAX_N_BODIES = 128, MAX_N_ACTORS = 64, MAX_N_PROJECTILES = 64 };

static struct archetype _archetypes[ARCHETYPE_LAST];
struct archetype *global_archetypes = _archetypes;

static enum handler_return handle_border_collision(struct ear *me __attribute__((unused)),
                                                   struct msg *msg)
{
    struct msg offside_msg = {.type = MSG_OFFSIDE};
    switch (msg->type) {
    case MSG_COLLISION:
        TELL(((struct collision_msg *)msg)->them->ear, &offside_msg);
        return STATE_HANDLED;
    default:
        return STATE_IGNORED;
    }
}

static struct {
    struct ear base;
    struct body *body;
} border;

static void construct_border(void)
{
    border.base.handler = handle_border_collision;
    float screen_radius = 10. + sqrtf(powf(viewport_w/2, 2)+powf(viewport_h/2,2));
    border.body = body_new(viewport_w/2. + I*(viewport_h/2.), screen_radius);
    border.body->affiliation = AFFILIATION_BORDER;
    border.body->flags |= COLLIDES_INVERSE;
    border.body->ear = &border.base;
}

#ifdef DEBUG
#include "draw.h"

static void draw_collision_region(struct body *b)
{
    draw_circle(b->p, b->collision_radius);
}
#endif

static enum outcome inner_game_loop(strand self, struct game *game)
{
    bodies_init(MAX_N_BODIES);
    projectiles_init(MAX_N_PROJECTILES);
    construct_border();

    actors_init(MAX_N_ACTORS, global_archetypes, ARCHETYPE_LAST);
    struct level *level = level_load(game->level);
    ENSURE(level);
    osd_init();

    enum outcome outcome = NO_OUTCOME;
    do {
        stage_draw();
        actors_draw();
        projectiles_draw();
#ifdef DEBUG
        bodies_foreach(draw_collision_region);
#endif
        osd_draw();

        float elapsed_time = strand_yield(self);
        stage_update(elapsed_time);
        bodies_update(elapsed_time);
        strand_resume(level->strand, elapsed_time);
        actors_update(elapsed_time);
        osd_update(elapsed_time);

        if (audio_is_available()) {
            music_update(elapsed_time);
            sfx_update(elapsed_time);
            audio_update(elapsed_time);
        }

        if (inputs[IN_MENU]) world_camera.scaling += 0.01f;
        if (inputs[IN_QUIT])
            outcome = OUTCOME_QUIT;

        if (!strand_is_alive(level->strand))
            outcome = OUTCOME_NEXT_LEVEL;
    } while (NO_OUTCOME == outcome);

    osd_destroy();
    level_destroy(level);
    actors_destroy();
    projectiles_destroy();
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
    new_game(self);
}

void game_entry_point(strand self)
{
    camera_init();
    tilemap_init();
    text_init();
    sprite_init();
    if (audio_init()) {
        sfx_init();
        music_init();
        sfx_enum all_sfx[] = { SFX_PLAYER_BULLET, SFX_SMALL_EXPLOSION, SFX_NONE };
        sfx_load(all_sfx);
    }
    main_menu_loop(self);
}
