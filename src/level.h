#pragma once

#include "strand.h"

struct level {
    strand strand;
};

extern struct level *level_load(unsigned level);
extern void level_destroy(struct level *level);

enum { N_LEVELS = 1 };
