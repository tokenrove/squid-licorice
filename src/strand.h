#pragma once

#include <stdlib.h>
#include <stdbool.h>

enum { STRAND_DEFAULT_STACK_SIZE = 64*1024 };

typedef void *strand;
extern strand strand_spawn_0(void (*fn)(strand), size_t size_in_words);
extern strand strand_spawn_1(void (*fn)(strand, void *), size_t size_in_words, void *argument);
extern void strand_resume(strand strand, float dt);
extern float strand_yield(strand self);
extern bool strand_is_alive(strand strand);
extern void strand_destroy(strand strand);
