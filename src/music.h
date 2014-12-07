#pragma once

#include <stdbool.h>

typedef void *music_handle;

extern music_handle music_load(const char *path);
extern void music_destroy(music_handle);

extern void music_init(void);
extern void music_update(float elapsed_time);
extern void music_play(music_handle);
extern void music_fade_out(float duration);
