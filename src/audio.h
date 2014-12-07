#pragma once

#include <stdbool.h>

extern bool audio_init(void);
extern bool audio_is_available(void);
extern void audio_update(float);
