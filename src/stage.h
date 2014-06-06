#pragma once

#include "strand.h"

struct stage {
    void (*fn)(strand);
    size_t fn_stack_size;
    strand strand;
};

extern struct stage *stage_load(int stage);
extern void stage_destroy(struct stage *stage);

enum { N_STAGES = 1 };
