
#include <GL/glew.h>
#include <GL/gl.h>

#include "ensure.h"
#include "shader.h"
#include "camera.h"
#include "point_sprite.h"
#include "log.h"

static const GLchar point_sprite_vertex_shader_src[] = {
#include "point_sprite.vert.i"
    , 0 };
static const GLchar point_sprite_fragment_shader_src[] = {
#include "point_sprite.frag.i"
    , 0 };
static GLuint shader, a_points;

void point_sprite_init(void)
{
    texture_init();
    if (0 == shader) {
        float ps[2];
        glGetFloatv(GL_POINT_SIZE_RANGE, ps);
        if (ps[1] < 16)
            LOG("The largest point sprite size available is %f, which is probably too small for things to work properly.", ps[1]);
        shader = build_shader_program("point_sprite", point_sprite_vertex_shader_src, point_sprite_fragment_shader_src);
    }
    ENSURE(shader);
    glGenBuffers(1, &a_points);
}

void point_sprite_draw_batch(struct point_sprite *s, size_t n, position *p)
{
    ENSURE(shader);

    glUseProgram(shader);

    glBindBuffer(GL_ARRAY_BUFFER, a_points);
    GLint points_atloc = glGetAttribLocation(shader, "a_point");
    glVertexAttribPointer(points_atloc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(points_atloc);
    glEnableClientState(GL_VERTEX_ARRAY);

    glUniform2f(glGetUniformLocation(shader, "u_translation"),
                crealf(world_camera.translation),
                cimagf(world_camera.translation));
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_projection"),
                       1, GL_FALSE, camera_projection_matrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s->atlas->id);
    glUniform1i(glGetUniformLocation(shader, "u_atlas"), 0);
    glUniform2f(glGetUniformLocation(shader, "u_texcoord"),
                (float)s->x / s->atlas->width,
                (float)s->y / s->atlas->height);
    glUniform2f(glGetUniformLocation(shader, "u_ratio"),
                (float)s->size / s->atlas->width,
                (float)s->size / s->atlas->height);
    glPointSize(s->size);

    glBufferData(GL_ARRAY_BUFFER, sizeof (position) * n, p, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_POINTS, 0, n);
}

#ifdef UNIT_TEST_POINT_SPRITE
#include "libtap/tap.h"
#include "video.h"
#include "camera.h"
#include "test_video.h"

/* Actually slightly wider to catch edge-of-screen issues. */
static position random_position_on_screen(void)
{
    return ((1.1 * drand48() - 0.05) * viewport_w) + I*((1.1 * drand48() - 0.05) * viewport_h);
}

static void test_basic_output(void)
{
    enum { FIXED_SEED = 424242 };
    struct point_sprite a, b;
    note("Testing basic shader output");
    note("Using fixed seed: srand48(%ld)", FIXED_SEED);
    srand48(FIXED_SEED);
    struct texture t;
    a = (struct point_sprite){.x = 0, .y = 0, .size = 16, .atlas = &t };
    b = (struct point_sprite){.x = 16, .y = 0, .size = 16, .atlas = &t };
    ok(texture_from_png(&t, "t/point_sprite.t-0.png"));
    video_start_frame();
    size_t n = 1024;
    position p[n];
    for (size_t i = 0; i < n; ++i)
        p[i] = random_position_on_screen();
    point_sprite_draw_batch(&a, n, p);
    for (size_t i = 0; i < n; ++i)
        p[i] = random_position_on_screen();
    point_sprite_draw_batch(&b, n, p);
    video_end_frame();
    todo("Unfortunately, OSMesa does not render textured point sprites correctly");
    ok(test_video_compare_fb_with_file("t/point_sprite.t-0.640x480.png"));
    end_todo;
    texture_destroy(&t);
}

int main(void)
{
    video_init();
    camera_init();
    point_sprite_init();
    plan(3);
    test_basic_output();
    done_testing();
}
#endif
