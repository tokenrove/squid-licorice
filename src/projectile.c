#include <stddef.h>
#include <math.h>

#include "projectile.h"
#include "ensure.h"
#include "game_constants.h"
#include "point_sprite.h"
#include "ensure.h"
#include "inline_math.h"
#include "physics.h"
#include "msg_macros.h"

static struct texture atlas;
static struct point_sprite sprites[PROJECTILE_LAST] = {
    { .atlas = &atlas, .x = 0, .y = 0, .size = 16 }
};
struct t {
    struct ear base;
    bool is_alive;
    struct body *body;
};
static size_t ring_size, producer_i, consumer_i;
static struct t *projectiles;
static position *pos_batch;


void projectiles_init(size_t n)
{
    point_sprite_init();
    ENSURE(texture_from_png(&atlas, "data/projectiles.png"));
    ring_size = closest_power_of_2(n);
    ENSURE(projectiles = calloc(ring_size, sizeof (*projectiles)));
    ENSURE(pos_batch = calloc(ring_size, sizeof (*pos_batch)));
    producer_i = consumer_i = 0;
}

void projectiles_destroy(void)
{
    free(projectiles);
    projectiles = NULL;
    ring_size = producer_i = consumer_i = 0;
    texture_destroy(&atlas);
}

static size_t projectiles_count(void)
{
    return ((consumer_i < producer_i) ? consumer_i + ring_size : consumer_i) - producer_i;
}

static inline size_t succ(size_t i) { return (i+1) & (ring_size-1); }

void projectiles_draw(void)
{
    size_t j = 0;
    for (size_t i = consumer_i; i != producer_i; i = succ(i))
        if (projectiles[i].is_alive)
            pos_batch[j++] = projectiles[i].body->p;
        else {
            if (projectiles[i].body)
                body_destroy(projectiles[i].body);
            projectiles[i].body = 0;
            if (consumer_i == i) consumer_i = succ(consumer_i);
        }
    if (j == 0) return;
    point_sprite_draw_batch(&sprites[0], j, pos_batch);
}

static enum handler_return handler(struct t *me, struct msg *m)
{
    switch(m->type) {
    case MSG_COLLISION: {
        struct collision_msg *cm = (struct collision_msg *)m;
        struct damage_msg dm = { .base.type = MSG_DAMAGE, .amount = 1 };
        TELL(cm->them->ear, &dm);
        me->is_alive = false;
        return STATE_HANDLED;
    }
    case MSG_OFFSIDE:
        /* we probably already died in the collision, but just in case */
        me->is_alive = false;
        return STATE_HANDLED;
    default:
        return STATE_IGNORED;
    }
}

static void reap(void)
{
    while (producer_i != consumer_i && !projectiles[consumer_i].is_alive) {
        body_destroy(projectiles[consumer_i].body);
        consumer_i = succ(consumer_i);
    }
}

bool projectile_shoot_at(position origin, position target,
                         enum projectile_type type,
                         unsigned affiliation)
{
    reap();
    struct t *this = &projectiles[producer_i];
    producer_i = succ(producer_i);
    if (producer_i == consumer_i && projectiles[consumer_i].is_alive) {
        body_destroy(projectiles[consumer_i].body);
        consumer_i = succ(consumer_i);
    }
    ENSURE(producer_i != consumer_i);

    this->body = body_new(origin, sprites[0].size);
    ENSURE(this->body);
    this->body->affiliation = affiliation;
    this->body->flags = COLLIDES_BY_AFFILIATION;
    this->base.handler = handler;
    this->body->ear = &this->base;
    position adjusted = target-origin;
    float theta = atan2f(cimagf(adjusted), crealf(adjusted));
    float speed = 42.;
    this->body->F = speed*cosf(theta) + I*speed*sinf(theta);
    this->is_alive = true;
    return true;
}

#ifdef UNIT_TEST_PROJECTILE
#include "libtap/tap.h"
#include "video.h"
#include "camera.h"
#include "physics.h"

static void projectile_fire_offside_verify_culled(void)
{
    bodies_init(2);
    projectiles_init(1);

    float screen_radius = 10. + sqrtf(powf(viewport_w/2, 2)+powf(viewport_h/2,2));
    struct body *border = body_new(viewport_w/2. + I*(viewport_h/2.), screen_radius);
    ok(NULL != border);
    border->flags |= COLLIDES_INVERSE;

    projectile_shoot_at(viewport_w/2 + I*(viewport_h/2), 0., PROJECTILE_BULLET, AFFILIATION_PLAYER);
    int n;
    for (n = 100; projectiles_count() > 0 && n > 0; --n)
        bodies_update(1.);
    cmp_ok(n, ">", 0);
    projectiles_destroy();
    bodies_destroy();
}

int main(void)
{
    video_init();
    camera_init();
    plan(2);
    todo();
    projectile_fire_offside_verify_culled();
    // create projectile and verify it gets culled when offside
    // create projectile and verify it collides with something else (verify collision modes)
    // verify pointsprite drawing
    end_todo;
    done_testing();
}
#endif
