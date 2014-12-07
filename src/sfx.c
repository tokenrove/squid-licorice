#include "SDL2/SDL_mixer.h"

#include "audio.h"
#include "ensure.h"
#include "log.h"
#include "sfx.h"

static Mix_Chunk *chunks[SFX_ENUM_LAST] = {0};
static const char *const paths[SFX_ENUM_LAST] = {
    "(none)",
    "sfx/player_bullet.ogg",
    "sfx/small_explosion.ogg"
};

void sfx_init(void)
{
    audio_init();
}

void sfx_update(float elapsed_time __attribute__ ((unused)))
{
}

bool sfx_load(sfx_enum *sounds)
{
    bool success = true;
    while (*sounds != SFX_NONE) {
        ENSURE(*sounds < SFX_ENUM_LAST);
        chunks[*sounds] = Mix_LoadWAV(paths[*sounds]);
        if (NULL == chunks[*sounds]) {
            success = false;
            LOG_ERROR("Unable to load sound %d: %s", *sounds, paths[*sounds]);
        }
        ++sounds;
    }
    return success;
}

sfx_handle sfx_play_oneshot(sfx_enum sound)
{
    ENSURE(sound > SFX_NONE && sound < SFX_ENUM_LAST);
    Mix_Chunk *c = chunks[sound];
    if (NULL == c) {
        LOG_DEBUG("Sound %d not loaded", sound);
        return -1;
    }
    return Mix_PlayChannel(-1, c, 0);
}
