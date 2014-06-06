#include <GL/glew.h>
#include <GL/gl.h>

#include <stdio.h>
#include <stdbool.h>

#include "ensure.h"
#include "shader.h"


static void log_shader_errors(const char *fn, GLuint object)
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
     fprintf(stderr, "%s: %s: %s", fn, name, log);
     free(log);
     return;
}

static int compile_shader(const char *name, const GLchar *src, GLenum type)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (GL_TRUE != compiled) {
        log_shader_errors(name, shader);
        return 0;
    }
    return shader;
}

GLuint build_shader_program(const char *name, const GLchar *vertex_shader_src, const GLchar *fragment_shader_src)
{
     GLuint vshader, fshader, program = 0;

     ENSURE(vshader = compile_shader(name, vertex_shader_src, GL_VERTEX_SHADER));
     ENSURE(fshader = compile_shader(name, fragment_shader_src, GL_FRAGMENT_SHADER));
     program = glCreateProgram();
     glAttachShader(program, vshader);
     glAttachShader(program, fshader);
     glLinkProgram(program);
     glDeleteShader(vshader);
     glDeleteShader(fshader);
     GLint linked;
     glGetProgramiv(program, GL_LINK_STATUS, &linked);
     if(GL_TRUE != linked) {
         log_shader_errors(name, program);
         goto on_error;
     }
     return program;
on_error:
     glDeleteProgram(program);
     return 0;
}
