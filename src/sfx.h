#pragma once

#include <stdbool.h>

typedef enum {
    SFX_NONE = 0,
    SFX_PLAYER_BULLET,
    SFX_SMALL_EXPLOSION,
    SFX_ENUM_LAST
} sfx_enum;

typedef int sfx_handle;

extern void sfx_init(void);
extern void sfx_update(float);
extern bool sfx_load(sfx_enum *);
extern sfx_handle sfx_play_oneshot(sfx_enum);
