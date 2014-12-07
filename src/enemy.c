#include "game.h"
#include "actor.h"
#include "msg_macros.h"
#include "projectile.h"
#include "log.h"
#include "sfx.h"

struct enemy_a {
    unsigned *group_ctr;
    float frame_ctr;
};

static enum handler_return enemy_explode(struct actor *me, struct msg *e)
{
    switch (e->type) {
    default: return STATE_IGNORED;
    case MSG_TICK: {
        struct enemy_a *state = me->state;
        state->frame_ctr -= ((struct tick_msg *)e)->elapsed_time;
        if (state->frame_ctr < 0)
            return XITION(NULL);
        return STATE_HANDLED;
    }
    case MSG_ENTER:
        me->sprite.x = 24;
        me->sprite.y = 0;
        me->sprite.w = 26;
        me->sprite.h = 24;
        ((struct enemy_a *)me->state)->frame_ctr = .5;
        sfx_play_oneshot(SFX_SMALL_EXPLOSION);
        return STATE_HANDLED;
    }
}

#include <math.h>

static void die(struct enemy_a *state)
{
    if (*state->group_ctr > 0)
        --*state->group_ctr;
    else
        LOG_DEBUG("Enemy's group counter already zero");
}

static enum handler_return enemy_a_initial(struct actor *me, struct msg *e)
{
    switch (e->type) {
    default: break;
    case MSG_TICK: {
        float t = ((struct tick_msg *)e)->elapsed_time;
        me->body->impulses = (100.*t*I) + 100.*t*sinf(cimagf(me->body->p)/100);
        return STATE_HANDLED;
    }
    case MSG_DAMAGE:
        die(me->state);
        return XITION(enemy_explode);
    case MSG_OFFSIDE:
        die(me->state);
        return XITION(NULL);
    case MSG_COLLISION: {
        struct damage_msg dm = { .base.type = MSG_DAMAGE, .amount = 1 };
        TELL(((struct collision_msg *)e)->them->ear, &dm);
        return STATE_HANDLED;
    }
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
        .atlas_path = "data/sprites.png",
        .collision_radius = 20.,
        .mass = 30.,
        .initial_handler = (msg_handler)enemy_a_initial,
        .state_size = sizeof (struct enemy_a)
    };
    global_archetypes[ARCHETYPE_CIRCLING_ENEMY] = (struct archetype){
        .atlas_path = "data/sprites.png",
        .collision_radius = 20.,
        .mass = 30.,
        .initial_handler = (msg_handler)enemy_a_initial,
        .state_size = sizeof (struct enemy_a)
    };
}
