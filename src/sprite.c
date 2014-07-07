
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
    glUniform1i(glGetUniformLocation(shader, "u_all_white"), s->all_white);

    const GLfloat vertices[] = { 0., 0., s->w, 0., s->w, s->h, 0., s->h };
    glBufferData(GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_DYNAMIC_DRAW);

    uint8_t indices[] = { 0,1,2, 2,3,0 };
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
}

#ifdef UNIT_TEST_SPRITE
#include "libtap/tap.h"
#include "video.h"
#include "camera.h"
#include "test_video.h"

static void test_basic_output(void)
{
    struct sprite sprite_a, sprite_b;
    note("Testing basic shader output");
    struct texture t;
    sprite_a = (struct sprite){.x = 0, .y = 0, .w = 16, .h = 16, .scaling = 1., .atlas = &t };
    sprite_b = (struct sprite){.x = 16, .y = 0, .w = 16, .h = 16, .scaling = 1., .atlas = &t };
    ok(texture_from_png(&t, "t/sprite.t-0.png"));
    video_start_frame();
    position p = I*64;
    sprite_draw(&sprite_a, p);
    sprite_draw(&sprite_a, 32.+p);
    sprite_draw(&sprite_b, 64.+p);
    sprite_b.all_white = true;
    sprite_draw(&sprite_b, 96.+p);
    sprite_b.all_white = false;
    p = I*128;
    for (int i = 0; i < viewport_w; ++i)
        sprite_draw((i%2) ? &sprite_a : &sprite_b, p+i);
    video_end_frame();
    ok(test_video_compare_fb_with_file("t/sprite.t-0.640x480.png"));
    texture_destroy(sprite_a.atlas);
}

int main(void)
{
    video_init();
    camera_init();
    sprite_init();
    plan(4);
    test_basic_output();
    done_testing();
}
#endif
