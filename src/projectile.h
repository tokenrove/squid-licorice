#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "geometry.h"
#include "msg.h"

enum projectile_type {
    PROJECTILE_BULLET,
    PROJECTILE_LAST
};

struct damage_msg {
    struct msg base;
    int amount;
};

extern void projectiles_init(size_t n);
extern void projectiles_destroy(void);
extern void projectiles_draw(void);

extern bool projectile_shoot_at(position origin, position target,
                                enum projectile_type type,
                                unsigned affiliation);

