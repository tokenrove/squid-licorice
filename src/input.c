
#include <SDL2/SDL.h>

#include "ensure.h"
#include "input.h"

/* Having this be global is lame, but sometimes that's how it is. */
input_state inputs[IN_LAST];

static input input_from_keysym(SDL_Keycode sym)
{
    switch (sym) {
    case SDLK_ESCAPE:
        return IN_QUIT;
    case SDLK_DOWN:
        return IN_DOWN;
    case SDLK_UP:
        return IN_UP;
    case SDLK_LEFT:
        return IN_LEFT;
    case SDLK_RIGHT:
        return IN_RIGHT;
    case SDLK_LSHIFT:
        return IN_SHOOT;
    case SDLK_TAB:
        return IN_MENU;
    default: break;
    };
    return IN_LAST;
}

void input_update(void)
{
    SDL_Event e;
    int watchdog = 100;  /* arbitrary number of events, in case of livelock */
    input b;

    while (SDL_PollEvent(&e) && --watchdog) {
        switch (e.type) {
        case SDL_KEYDOWN:
            if (e.key.repeat) break;  /* ignore repeat events; fall-through */
        case SDL_KEYUP:
            b = input_from_keysym(e.key.keysym.sym);
            if (IN_LAST != b)
                inputs[b] = e.key.state ? JUST_PRESSED : NOT_PRESSED;
            break;
        case SDL_QUIT:
            inputs[IN_QUIT] = JUST_PRESSED;
            break;
        default: /* ignore */ break;
        }
    }
    ENSURE(watchdog);
}

const char *input_get_readable_event_name_for_binding(input i)
{
    return "unknown";
}

void input_rebind_and_save(input i)
{
}
