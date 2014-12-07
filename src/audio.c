#include "log.h"
#include "audio.h"
#include "SDL2/SDL_mixer.h"

static bool is_available = false;

bool audio_init(void)
{
    if (is_available) return true;

    int got = Mix_Init(MIX_INIT_OGG);
    if (!(got & MIX_INIT_OGG)) return false;
    atexit(Mix_Quit);
    // XXX: This should be configurable.
    if (0 == Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096))
        is_available = true;
    return is_available;
}

bool audio_is_available(void)
{
    return is_available;
}

void audio_update(float elapsed_time __attribute__ ((unused)))
{
    /* SDL2_mixer does everything for us in another thread. */
}
