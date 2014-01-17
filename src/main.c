#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

#include "ensure.h"
#include "tilemap.h"

SDL_Window *sdl_window;

int main(/* int argc, char **argv */)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    atexit(SDL_Quit);

    sdl_window = SDL_CreateWindow("Squid Licorice",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  1280, 720,
                                  SDL_WINDOW_OPENGL);
    if (!sdl_window) return EXIT_FAILURE;
    SDL_GLContext gl_context = SDL_GL_CreateContext(sdl_window);
    if (NULL == gl_context) return EXIT_FAILURE;
    SDL_GL_SetSwapInterval(1);  // return value ignored

    if (GLEW_OK != glewInit()) return EXIT_FAILURE;

    // parameterize behavior by extensions supported here

    // setup the display for the usual 2D drawing
    // setup initial orthographic projection
    struct { GLint x, y, w, h; } viewport;
    glGetIntegerv(GL_VIEWPORT, (GLint*)&viewport);
    GLfloat projection_matrix[16] = { // column-oriented
        2./viewport.w, 0., 0., 0.,
        0., -2./viewport.h, 0., 0.,
        0., 0., -2./2., 0.,
        -1., 1., -1., 1.
    };
    GLfloat mv_matrix[16] = {
        1., 0., 0., 0.,
        0., 1., 0., 0.,
        0., 0., 1., 0.,
        0., 0., 0., 1.
    };

    // setup GL for sprites
    glDisable(GL_DEPTH_TEST);

    struct tilemap t;
    ENSURE(tilemap_load("map", "atlas.png", &t));

    while (1) {
        glClearColor(.8, .6, .5, 1.);
        glClear(GL_COLOR_BUFFER_BIT);

        tilemap_draw(&t, mv_matrix, projection_matrix);

        SDL_GL_SwapWindow(sdl_window);

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_KEYDOWN:
                if (e.key.state == SDL_PRESSED && e.key.keysym.sym == SDLK_ESCAPE)
                    SDL_PushEvent((SDL_Event*)&(SDL_QuitEvent){.type=SDL_QUIT});
                break;
            case SDL_QUIT:
                return EXIT_SUCCESS;
            }
        }
    }

    tilemap_destroy(&t);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(sdl_window);
    return 0;
}
