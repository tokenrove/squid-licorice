#pragma once

#include "strand.h"

struct level {
    void (*fn)(strand);
    size_t fn_stack_size;
    strand strand;
};

extern struct level *level_load(int level);
extern void level_destroy(struct level *level);

enum { N_LEVELS = 1 };
