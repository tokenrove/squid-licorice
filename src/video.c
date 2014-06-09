#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdbool.h>

#include "ensure.h"
#include "video.h"
#include "camera.h"
#include "log.h"

static SDL_Window *sdl_window;
static SDL_GLContext gl_context;
uint16_t viewport_w, viewport_h;

static void video_atexit()
{
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(sdl_window);
}

static void sdl_init(void)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    atexit(SDL_Quit);
    log_init();
}

void video_init(void)
{
    static bool is_initialized = false;

    if (is_initialized) return;
    is_initialized = true;

    sdl_init();

    ENSURE(sdl_window = SDL_CreateWindow("Squid Licorice",
                                         SDL_WINDOWPOS_UNDEFINED,
                                         SDL_WINDOWPOS_UNDEFINED,
                                         640, 480,
                                         SDL_WINDOW_OPENGL));
    ENSURE(gl_context = SDL_GL_CreateContext(sdl_window));
    SDL_GL_SetSwapInterval(1);  // return value ignored

    atexit(video_atexit);

    ENSURE(GLEW_OK == glewInit());

    // XXX parameterize behavior by extensions supported here

    struct { GLint x, y, w, h; } viewport;
    glGetIntegerv(GL_VIEWPORT, (GLint*)&viewport);
    ENSURE(0 == viewport.x && 0 == viewport.y);
    viewport_w = viewport.w; viewport_h = viewport.h;

    glDisable(GL_DEPTH_TEST);
    //glClearColor(.8, .6, .5, 1.);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void video_start_frame(void) { glClear(GL_COLOR_BUFFER_BIT); }
void video_end_frame(void) { SDL_GL_SwapWindow(sdl_window); }
