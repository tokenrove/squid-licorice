
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "ensure.h"
#include "input.h"
#include "log.h"

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

    if (0 == SDL_NumJoysticks()) {
        LOG("No joysticks detected (see https://hg.libsdl.org/SDL/file/e3e00f8e6b91/README-linux.txt if this is erroneous).");
    }

    SDL_JoystickEventState(SDL_ENABLE);
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameController *controller = SDL_GameControllerOpen(i);
            if (controller)
                LOG_DEBUG("Opened game controller: %s", SDL_GameControllerName(controller));
            else
                LOG_DEBUG("Found a game controller (%s) but couldn't open it.",
                    SDL_GameControllerNameForIndex(i));
        } else {
            SDL_Joystick *joy = SDL_JoystickOpen(i);
            if (joy)
                LOG_DEBUG("Opened joystick: %s", SDL_JoystickNameForIndex(0));
        }
    }
}

void input_init(void)
{
    setup_joysticks();
}

static SDL_GameControllerButton controller_button_from_joystick(uint8_t button)
{
    switch (button) {
    case 9:
        return SDL_CONTROLLER_BUTTON_START;
    case 8:
        return SDL_CONTROLLER_BUTTON_GUIDE;
    case 1:
    case 7:
        return SDL_CONTROLLER_BUTTON_X;
    default:
        return SDL_CONTROLLER_BUTTON_INVALID;
    }
}

static input input_from_controller_button(SDL_GameControllerButton b)
{
    switch (b) {
    case SDL_CONTROLLER_BUTTON_START:
        return IN_MENU;
    case SDL_CONTROLLER_BUTTON_BACK:
    case SDL_CONTROLLER_BUTTON_GUIDE:
        return IN_QUIT;
    case SDL_CONTROLLER_BUTTON_X:
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        return IN_SHOOT;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
        return IN_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        return IN_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        return IN_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        return IN_RIGHT;
    default:
        return IN_LAST;
    }
}

static void debounce(void)
{
    for (int i = 0; i < IN_LAST; ++i)
        if (inputs[i]) inputs[i] = PRESSED;
}

static void update_by_axis_motion(int16_t value, bool is_vertical)
{
    const uint16_t joystick_dead_zone = 4200;
    bool livep = abs(value) >= joystick_dead_zone,
         plusp = value > 0;
    inputs[is_vertical ? IN_DOWN : IN_RIGHT] = (livep && plusp);
    inputs[is_vertical ? IN_UP : IN_LEFT] = (livep && !plusp);
}

static void update_by_button(SDL_GameControllerButton button, bool state)
{
    input b = input_from_controller_button(button);
    if (IN_LAST != b)
        inputs[b] = state ? JUST_PRESSED : NOT_PRESSED;
}

void input_update(void)
{
    SDL_Event e;
    int watchdog = 100;  /* arbitrary number of events, in case of livelock */
    input b;

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
        case SDL_JOYBUTTONUP:
            update_by_button(controller_button_from_joystick(e.jbutton.button), e.jbutton.state);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            update_by_button(e.cbutton.button, e.cbutton.state);
            break;
        case SDL_CONTROLLERAXISMOTION:
            if (SDL_CONTROLLER_AXIS_LEFTX == e.caxis.axis)
                update_by_axis_motion(e.caxis.value, false);
            else if (SDL_CONTROLLER_AXIS_LEFTY == e.caxis.axis)
                update_by_axis_motion(e.caxis.value, true);
            break;
        case SDL_JOYAXISMOTION:
            update_by_axis_motion(e.jaxis.value, 1 == e.jaxis.axis);
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
