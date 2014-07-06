#include "game.h"
#include "actor.h"
#include "msg_macros.h"
#include "input.h"

struct player {
    // weapons etc
};

static enum handler_return player_initial(struct actor *me, struct msg *e)
{
    switch (e->type) {
    case MSG_ENTER:
        me->body->affiliation = AFFILIATION_PLAYER;
        me->sprite.x = 0;
        me->sprite.y = 0;
        me->sprite.w = 29;
        me->sprite.h = 51;
        return STATE_HANDLED;
    case MSG_TICK:
        if (inputs[IN_UP])
            me->body->impulses += -5.*inputs[IN_UP]*I;
        if (inputs[IN_DOWN])
            me->body->impulses += 5.*inputs[IN_DOWN]*I;
        if (inputs[IN_LEFT])
            me->body->impulses += -5.*inputs[IN_LEFT];
        if (inputs[IN_RIGHT])
            me->body->impulses += 5.*inputs[IN_RIGHT];
        return STATE_HANDLED;
    case MSG_OFFSIDE:
    case MSG_COLLISION:
    default:
        return STATE_IGNORED;
    }
}


__attribute__ ((constructor))
static void populate_archetypes(void)
{
    global_archetypes[ARCHETYPE_PLAYER1] = (struct archetype){
        .atlas_path = "sprites.png",
        .collision_radius = 20.,
        .mass = 30.,
        .initial_handler = (msg_handler)player_initial,
        .state_size = sizeof (struct player)
    };
}
