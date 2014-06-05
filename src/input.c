
#include <stdbool.h>

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

static void setup_joysticks()
{
    SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");

    if (0 == SDL_NumJoysticks())
        SDL_LogInfo(SDL_LOG_CATEGORY_INPUT,
                    "No joysticks detected (see https://hg.libsdl.org/SDL/file/e3e00f8e6b91/README-linux.txt if this is erroneous).");

    SDL_Joystick *joy = NULL;

    SDL_JoystickEventState(SDL_ENABLE);
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            SDL_LogDebug(SDL_LOG_CATEGORY_INPUT,
                         "Found a game controller (%s) but decided to ignore it.",
                         SDL_GameControllerNameForIndex(i));
        }
        joy = SDL_JoystickOpen(i);
        if (joy)
            SDL_LogDebug(SDL_LOG_CATEGORY_INPUT,
                         "Opened joystick: %s", SDL_JoystickNameForIndex(0));
    }
}

void input_init(void)
{
    setup_joysticks();
}

static input input_from_joystick_button(SDL_JoyButtonEvent e)
{
    switch (e.button) {
    case 0:
        return IN_MENU;
    case 8:
        return IN_QUIT;
    case 1:
        return IN_SHOOT;
    default:
        return IN_LAST;
    }
}

static void debounce(void)
{
    for (int i = 0; i < IN_LAST; ++i)
        if (inputs[i]) inputs[i] = PRESSED;
}

void input_update(void)
{
    SDL_Event e;
    int watchdog = 100;  /* arbitrary number of events, in case of livelock */
    input b;
    const uint16_t joystick_dead_zone = 4200;

    debounce();

    while (SDL_PollEvent(&e) && --watchdog) {
        switch (e.type) {
        case SDL_KEYDOWN:
            if (e.key.repeat) break;  /* ignore repeat events; fall-through */
        case SDL_KEYUP:
            b = input_from_keysym(e.key.keysym.sym);
            if (IN_LAST != b)
                inputs[b] = e.key.state ? JUST_PRESSED : NOT_PRESSED;
            break;
        case SDL_JOYBUTTONDOWN:
            b = input_from_joystick_button(e.jbutton);
            if (IN_LAST != b)
                inputs[b] = e.jbutton.state ? JUST_PRESSED : NOT_PRESSED;
            break;
        case SDL_JOYAXISMOTION: ;
            bool deadp = abs(e.jaxis.value) < joystick_dead_zone,
                 plusp = e.jaxis.value > 0,
             verticalp = e.jaxis.axis == 1;
            inputs[verticalp ? IN_UP : IN_LEFT] = (deadp || !plusp);
            inputs[verticalp ? IN_DOWN : IN_RIGHT] = (deadp || plusp);
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
