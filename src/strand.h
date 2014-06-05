#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef void *strand;
extern strand strand_spawn(void (*fn)(strand), size_t size_in_words);
extern void strand_resume(strand strand, float dt);
extern float strand_yield(strand self);
extern bool strand_is_alive(strand strand);
