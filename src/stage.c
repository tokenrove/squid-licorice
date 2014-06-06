
#include <stdlib.h>

#include "stage.h"
#include "ensure.h"
#include "strand.h"

#include <SDL2/SDL.h>

static void stage1_entry(strand self)
{
    float accum = 0.;
    while(1) {
        accum += strand_yield(self);
        if (accum > 5.f) {
            SDL_Log("stage 1; %f\n", accum);
            accum -= 5.;
        }
    }
}


static struct {
    void (*fn)(strand);
    size_t fn_stack_size;
} stages[N_STAGES] = {
    { .fn = stage1_entry, .fn_stack_size = STRAND_DEFAULT_STACK_SIZE }
};

struct stage *stage_load(int i)
{
    struct stage *stage;
    stage = calloc(1, sizeof (*stage));
    ENSURE(stage);
    stage->strand = strand_spawn(stages[i].fn, stages[i].fn_stack_size);
    return stage;
}

void stage_destroy(struct stage *stage)
{
    strand_destroy(stage->strand);
    free(stage);
}
