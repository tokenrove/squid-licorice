
#include <stdlib.h>

#include "level.h"
#include "ensure.h"
#include "strand.h"
#include "stage.h"

#include <SDL2/SDL.h>


// wait for distance
// wait for chunk list exceeded
// wait for time elapsed
// wait for enemies cleared

/* 
 * static void wait_distance(strand self, float screens)
 * {
 *     uint32_t start = scroll_distance_travelled;
 *     while (scroll_distance_travelled - start < (viewport_h * screens))
 *         strand_yield(self);
 * }
 */

static void level1_entry(strand self)
{
    // load chunks

    /* 
     * scroll_distance_travelled = 0;
     * //start_music(level1_music);
     * //set fragment to loop loop(map_fragment_1);
     * scroll_set_loop(named_chunk, true);
     * spawn_player();
     * scroll_set_speed(1/10.);
     * osd_display_message(5.0, "Level 1: Find the Rum!");
     * wait_distance(1.0);
     * scroll_set_loop(named_chunk, false);
     * 
     * spawn_squadron(fish);
     * spawn_actor(clam, x, y);
     * wait_chunks_exhausted();
     */
    while (1) strand_yield(self);
}


static struct {
    void (*fn)(strand);
    size_t fn_stack_size;
} levels[N_LEVELS] = {
    { .fn = level1_entry, .fn_stack_size = STRAND_DEFAULT_STACK_SIZE }
};

struct level *level_load(int i)
{
    stage_start();
    struct level *level;
    level = calloc(1, sizeof (*level));
    ENSURE(level);
    level->strand = strand_spawn(levels[i].fn, levels[i].fn_stack_size);
    return level;
}

void level_destroy(struct level *level)
{
    stage_end();
    strand_destroy(level->strand);
    free(level);
}
