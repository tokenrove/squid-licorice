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

void log_init(void)
{
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
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
     SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "%s: %s: %s", fn, name, log);
     free(log);
     return;
}

static const char *level_tag[LOG_LEVEL_ERROR+1] = { "DEBUG", "INFO", "ERROR" };

void log_msg(const char *fn, enum log_level level, const char *fmt, ...)
{
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
