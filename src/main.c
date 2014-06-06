#include <SDL2/SDL.h>

#include "ensure.h"
#include "video.h"
#include "timer.h"
#include "strand.h"
#include "input.h"
#include "game.h"

double average_frame_time;

static inline float max(const float a, const float b) { return a < b ? b : a; }

static float update_frame_timer(void)
{
    const float target_frame_time = 1./60., alpha = 1/10.;
    static uint32_t last_ticks = 0;
    uint32_t now = timer_ticks_ms();
    float elapsed_time = (now - last_ticks) / 1000.;
    last_ticks = now;
    average_frame_time += alpha * (elapsed_time - average_frame_time);
    /* The first time through this routine, or if the machine is
     * performing egregiously poorly, elapsed_time will be very large,
     * so we'll pretend it was a normal frame. */
    if (elapsed_time > 3.f*target_frame_time)
        elapsed_time = target_frame_time;
    /* After a timer overflow, elapsed_time will be negative, and so
     * we'll sleep to around the target frame time.  This is also
     * avoids very small time deltas, in the case of a very fast
     * machine (or logic error somewhere), which could cause numerical
     * instability elsewhere. */
    if (elapsed_time < target_frame_time/4.f) {
        timer_sleep_ms(target_frame_time - max(0., elapsed_time));
        elapsed_time = target_frame_time;
    }
    return elapsed_time;
}

int main(/* int argc, char **argv */)
{
    video_init();
    input_init();

    strand game_strand;
    // XXX how much stack do we need?  This stack needs to be quite
    // large because things like shaders get compiled on it.
    game_strand = strand_spawn(game_entry_point, 256*1024);

    do {
        float elapsed_time = update_frame_timer();

        video_start_frame();
        strand_resume(game_strand, elapsed_time);
        video_end_frame();
        input_update();
    } while(strand_is_alive(game_strand));

    return 0;
}
