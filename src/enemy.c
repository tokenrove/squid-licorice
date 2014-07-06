#include "game.h"
#include "actor.h"
#include "msg_macros.h"
#include "log.h"

struct enemy_a {
    unsigned *group_ctr;
};

static enum handler_return enemy_a_initial(struct actor *me, struct msg *e)
{
    switch (e->type) {
    default: break;
    case MSG_TICK: return STATE_HANDLED;
    case MSG_OFFSIDE:
        LOG("%p went offside", me);
        return XITION(NULL);
    case MSG_ENTER:
        me->body->affiliation = AFFILIATION_ENEMY;
        me->sprite.x = 58;
        me->sprite.y = 0;
        me->sprite.w = 26;
        me->sprite.h = 24;
        return STATE_HANDLED;
    }
    return STATE_IGNORED;
}

__attribute__ ((constructor))
static void populate_archetypes(void)
{
    global_archetypes[ARCHETYPE_WAVE_ENEMY] = (struct archetype){
        .atlas_path = "sprites.png",
        .collision_radius = 20.,
        .mass = 30.,
        .initial_handler = (msg_handler)enemy_a_initial,
        .state_size = sizeof (struct enemy_a)
    };
    global_archetypes[ARCHETYPE_CIRCLING_ENEMY] = (struct archetype){
        .atlas_path = "sprites.png",
        .collision_radius = 20.,
        .mass = 30.,
        .initial_handler = (msg_handler)enemy_a_initial,
        .state_size = sizeof (struct enemy_a)
    };
}
