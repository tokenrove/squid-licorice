#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

#include "ensure.h"
#include "video.h"
#include "camera.h"

static SDL_Window *sdl_window;
static SDL_GLContext gl_context;
static struct { GLint x, y, w, h; } viewport;

static void video_atexit()
{
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(sdl_window);
}

void video_init(void)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    atexit(SDL_Quit);

    ENSURE(sdl_window = SDL_CreateWindow("Squid Licorice",
                                         SDL_WINDOWPOS_UNDEFINED,
                                         SDL_WINDOWPOS_UNDEFINED,
                                         1280, 720,
                                         SDL_WINDOW_OPENGL));
    ENSURE(gl_context = SDL_GL_CreateContext(sdl_window));
    SDL_GL_SetSwapInterval(1);  // return value ignored

    atexit(video_atexit);

    ENSURE(GLEW_OK == glewInit());

    // parameterize behavior by extensions supported here

    // setup the display for the usual 2D drawing
    // setup initial orthographic projection
    // setup GL for sprites
    glGetIntegerv(GL_VIEWPORT, (GLint*)&viewport);
    ENSURE(0 == viewport.x && 0 == viewport.y);
    camera_set_ortho_projection(viewport.w, viewport.h);

    glDisable(GL_DEPTH_TEST);
    //glClearColor(.8, .6, .5, 1.);
}

void video_start_frame(void) { glClear(GL_COLOR_BUFFER_BIT); }
void video_end_frame(void) { SDL_GL_SwapWindow(sdl_window); }
