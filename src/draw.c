
#include <GL/glew.h>
#include <GL/gl.h>

#include "ensure.h"
#include "shader.h"
#include "camera.h"
#include "draw.h"

#include <math.h>


static const GLchar draw_vertex_shader_src[] = {
#include "draw.vert.i"
    , 0 };
static const GLchar draw_fragment_shader_src[] = {
#include "draw.frag.i"
    , 0 };
static GLuint shader, a_vertices;

void draw_init(void)
{
    if (0 == shader)
        shader = build_shader_program("draw", draw_vertex_shader_src, draw_fragment_shader_src);
    ENSURE(shader);
    glGenBuffers(1, &a_vertices);
}

/* per http://slabode.exofire.net/circle_draw.shtml */
void draw_circle(position center, float radius)
{
    if (!shader) draw_init();
    int n = (10*radius);
    float theta = (2*M_PI) / n;
    float c = cosf(theta);
    float s = sinf(theta);
    float t;

    float x = radius;
    float y = 0;

    float vertices[2*n];
    for (int i = 0; i < n; ++i)
    {
        vertices[2*i] = x + crealf(center);
        vertices[2*i+1] = y + cimagf(center);
        t = x;
        x = c * x - s * y;
        y = s * t + c * y;
    }

    ENSURE(shader);

    glUseProgram(shader);

    glBindBuffer(GL_ARRAY_BUFFER, a_vertices);
    GLint vertices_atloc = glGetAttribLocation(shader, "a_vertex");
    glVertexAttribPointer(vertices_atloc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertices_atloc);
    glEnableClientState(GL_VERTEX_ARRAY);

    glUniform2f(glGetUniformLocation(shader, "u_translation"),
                crealf(world_camera.translation),
                cimagf(world_camera.translation));
    glUniform1f(glGetUniformLocation(shader, "u_scaling"),
                1.);
    glUniform1f(glGetUniformLocation(shader, "u_rotation"),
                0.);
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_projection"),
                       1, GL_FALSE, camera_projection_matrix);

    glUniform4f(glGetUniformLocation(shader, "u_color"), 1., 1., 1., 1.);
    glBufferData(GL_ARRAY_BUFFER, sizeof (float) * 2 * n, vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINE_LOOP, 0, n);
}
