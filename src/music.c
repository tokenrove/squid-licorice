/* music playback
 *
 * Music is a singleton because we don't need to play multiple tunes
 * at once.  We could abstract
 */
#include "audio.h"
#include "music.h"
#include "SDL2/SDL_mixer.h"

music_handle music_load(const char *path)
{
    Mix_Music *m = Mix_LoadMUS(path);
    return m;
}

void music_destroy(music_handle m)
{
    Mix_FreeMusic((Mix_Music *)m);
}

void music_init(void)
{
    audio_init();
}

void music_update(float elapsed_time __attribute__ ((unused)))
{
}

void music_play(music_handle m)
{
    Mix_PlayMusic((Mix_Music *)m, -1);
}

// a way to register that a msg gets sent when the music finishes
// Mix_HookMusicFinished

void music_fade_out(float duration)
{
    Mix_FadeOutMusic(duration * 1000.);
}
