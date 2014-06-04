#include <SDL2/SDL.h>

#include "timer.h"

uint32_t timer_ticks_ms(void)
{
    return SDL_GetTicks();
}

extern void timer_sleep_ms(uint32_t ticks)
{
    SDL_Delay(ticks);
}
