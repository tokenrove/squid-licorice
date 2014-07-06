
#include <GL/glew.h>
#include <GL/gl.h>

#include "ensure.h"
#include "shader.h"
#include "camera.h"
#include "sprite.h"

static const GLchar sprite_vertex_shader_src[] = {
#include "sprite.vert.i"
    , 0 };
static const GLchar sprite_fragment_shader_src[] = {
#include "sprite.frag.i"
    , 0 };
static GLuint shader, a_vertices;

void sprite_init(void)
{
    texture_init();
    if (0 == shader)
        shader = build_shader_program("sprite", sprite_vertex_shader_src, sprite_fragment_shader_src);
    ENSURE(shader);
    glGenBuffers(1, &a_vertices);
}

void sprite_draw(struct sprite *s, position p)
{
    ENSURE(shader);

    glUseProgram(shader);

    glBindBuffer(GL_ARRAY_BUFFER, a_vertices);
    GLint vertices_atloc = glGetAttribLocation(shader, "a_vertex");
    glVertexAttribPointer(vertices_atloc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertices_atloc);
    glEnableClientState(GL_VERTEX_ARRAY);

    p += world_camera.translation;
    glUniform2f(glGetUniformLocation(shader, "u_translation"),
                crealf(p) - s->w/2,
                cimagf(p) - s->h/2);
    glUniform1f(glGetUniformLocation(shader, "u_scaling"),
                s->scaling);
    glUniform1f(glGetUniformLocation(shader, "u_rotation"),
                s->rotation);
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_projection"),
                       1, GL_FALSE, camera_projection_matrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s->atlas->id);
    glUniform1i(glGetUniformLocation(shader, "u_atlas"), 0);
    glUniform4f(glGetUniformLocation(shader, "u_clip"),
                s->x, s->y, s->atlas->width, s->atlas->height);

    const GLfloat vertices[] = { 0., 0., s->w, 0., s->w, s->h, 0., s->h };
    glBufferData(GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_DYNAMIC_DRAW);

    uint8_t indices[] = { 0,1,2, 2,3,0 };
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
}

#ifdef UNIT_TEST_SPRITE
#include "libtap/tap.h"

int main(void)
{
    plan(1);
    todo();
    pass();
    end_todo;
    done_testing();
}
#endif
