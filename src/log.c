#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL2/SDL.h>

#include "log.h"
#include "ensure.h"

static enum log_level current;

static enum log_level our_priority_from_sdl_priority(SDL_LogPriority p)
{
    switch (p) {
    case SDL_LOG_PRIORITY_VERBOSE:
    case SDL_LOG_PRIORITY_DEBUG:
        return LOG_LEVEL_DEBUG;
    case SDL_LOG_PRIORITY_WARN:
    case SDL_LOG_PRIORITY_INFO:
        return LOG_LEVEL_INFO;
    case SDL_LOG_PRIORITY_ERROR:
    case SDL_LOG_PRIORITY_CRITICAL:
    default:
        return LOG_LEVEL_ERROR;
    }
}

static const char *sdl_category_to_string(int c)
{
    switch (c) {
    case SDL_LOG_CATEGORY_APPLICATION: return "APPLICATION";
    case SDL_LOG_CATEGORY_ERROR: return "ERROR";
    case SDL_LOG_CATEGORY_ASSERT: return "ASSERT";
    case SDL_LOG_CATEGORY_SYSTEM: return "SYSTEM";
    case SDL_LOG_CATEGORY_AUDIO: return "AUDIO";
    case SDL_LOG_CATEGORY_VIDEO: return "VIDEO";
    case SDL_LOG_CATEGORY_RENDER: return "RENDER";
    case SDL_LOG_CATEGORY_INPUT: return "INPUT";
    case SDL_LOG_CATEGORY_TEST: return "TEST";
    default: return "UNKNOWN";
    }
}

static void handle_sdl_log(void *data __attribute__ ((unused)),
                           int category,
                           SDL_LogPriority priority,
                           const char *message)
{
    enum log_level p = our_priority_from_sdl_priority(priority);
    log_msg("SDL", p, "[%s]: %s", sdl_category_to_string(category), message);
}

void log_init(void)
{
    SDL_LogSetOutputFunction(handle_sdl_log, NULL);
#ifdef DEBUG
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    current = LOG_LEVEL_DEBUG;
#else
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);
    current = LOG_LEVEL_INFO;
#endif
}

void log_noisy(void)
{
    current = LOG_LEVEL_DEBUG;
}

void log_quiet(void)
{
    current = LOG_LEVEL_ERROR;
}

void log_shader_errors(const char *fn, unsigned object)
{
    struct {
        void (*get)(GLuint, GLenum, GLint*);
        void (*log)(GLuint, GLsizei, GLsizei*, GLchar*);
    } fns[] = { { .get = glGetShaderiv, .log = glGetShaderInfoLog },
                { .get = glGetProgramiv, .log = glGetProgramInfoLog } };
    bool is_program = glIsProgram(object);

    GLsizei len;
    (*fns[is_program].get)(object, GL_INFO_LOG_LENGTH, &len);
    ENSURE(len > 0);
    GLchar *log = malloc(len * sizeof (GLchar));
    ENSURE(log);
    (*fns[is_program].log)(object, len, NULL, log);
    GLint type;
    const char *name;
    if(is_program)
        name = "program";
    else {
        glGetShaderiv(object, GL_SHADER_TYPE, &type);
        name = type == GL_VERTEX_SHADER ? "vertex" : "fragment";
    }
    log_msg(fn, LOG_LEVEL_ERROR, "%s: %s", name, log);
    free(log);
    return;
}

static const char *level_tag[LOG_LEVEL_ERROR+1] = { "DEBUG", "INFO", "ERROR" };

void log_msg(const char *fn, enum log_level level, const char *fmt, ...)
{
    if (level < current) return;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "%s: [%s]: ", fn, level_tag[level]);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
}

void log_lib_errors(const char *fn)
{
    if(errno) {
        log_msg(fn, LOG_LEVEL_ERROR, "errno: %s", strerror(errno));
        errno = 0;
    }
    const char *s = SDL_GetError();
    if(s && s[0]) {
        log_msg(fn, LOG_LEVEL_ERROR, "SDL: %s", s);
        SDL_ClearError();
    }
    GLenum e = glGetError();
    if(e)
        log_msg(fn, LOG_LEVEL_ERROR, "GL: %s", gluErrorString(e));
}
