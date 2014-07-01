#include <stdio.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include <arpa/inet.h>

#include "ensure.h"
#include "shader.h"
#include "tilemap.h"
#include "camera.h"

/* Shaders for classic tilemap rendering.
 *
 * We pass in the map and atlas dimensions in u_{map,atlas}_dims, and we
 * assume that the tiles are square and the atlas width is equal to
 * the width and height of a tile.
 *
 * Work could be shuffled around here so that the vertex shader does a
 * bit more of the work, which might be more efficient.  I doubt it
 * matters much, though.
 */
static const GLchar tilemap_vertex_shader_src[] = {
#include "tilemap.vert.i"
    , 0 };
static const GLchar tilemap_fragment_shader_src[] = {
#include "tilemap.frag.i"
    , 0 };
static GLuint shader, a_vertices;

void tilemap_init(void)
{
    texture_init();
    if (0 == shader)
        shader = build_shader_program("tilemap", tilemap_vertex_shader_src, tilemap_fragment_shader_src);
    ENSURE(shader);
    glGenBuffers(1, &a_vertices);
}

static void load_map(struct tilemap *t, const char *path)
{
    FILE *fp = fopen(path, "r");
    ENSURE(fp);
    uint8_t buffer[4];
    ENSURE(4 == fread(buffer, 1, 4, fp));
    for (int i = 0; i < 4; ++i) ENSURE(buffer[i] == "MAP\x01"[i]);
    uint16_t width, height;
    uint8_t *data;
    ENSURE(1 == fread(&width, 2, 1, fp));
    width = ntohs(width);
    ENSURE(1 == fread(&height, 2, 1, fp));
    height = ntohs(height);
    size_t n = width*height;
    ENSURE(data = malloc(n));
    ENSURE(fread(data, 1, n, fp) == n);
    fclose(fp);
    ENSURE(texture_from_pels(&t->map.texture, data, width, height, 1));
    free(data);
}

bool tilemap_load(const char *map_path, const char *atlas_path, struct tilemap *t)
{
    memset(t, 0, sizeof (*t));

    load_map(t, map_path);
    ENSURE(texture_from_png(&t->atlas.texture, atlas_path));

    uint16_t tile_width = t->atlas.texture.width;
    uint16_t tile_height = tile_width; // XXX we assume it's the same, here
    t->map.w = (float)t->map.texture.width*tile_width;
    t->map.h = (float)t->map.texture.height*tile_height;
    t->atlas.w = (float)t->atlas.texture.width;
    t->atlas.h = (float)t->atlas.texture.height;

    const GLfloat vertices[] = { 0., 0., t->map.w, 0., t->map.w, t->map.h, 0., t->map.h };
    glBindBuffer(GL_ARRAY_BUFFER, a_vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_DYNAMIC_DRAW);

    return true;
}

void tilemap_draw(struct tilemap *t, position offset)
{
    ENSURE(shader);
    glUseProgram(shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t->atlas.texture.id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, t->map.texture.id);

    glBindBuffer(GL_ARRAY_BUFFER, a_vertices);
    GLint vertices_atloc = glGetAttribLocation(shader, "a_vertex");
    glVertexAttribPointer(vertices_atloc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertices_atloc);

    glUniform2f(glGetUniformLocation(shader, "u_translation"),
                crealf(offset),
                cimagf(offset));
    glUniform1f(glGetUniformLocation(shader, "u_scaling"),
                world_camera.scaling);
    glUniform1f(glGetUniformLocation(shader, "u_rotation"),
                world_camera.rotation);
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_projection"),
                       1, GL_FALSE, camera_projection_matrix);

    glUniform1i(glGetUniformLocation(shader, "u_atlas"), 0);
    glUniform1i(glGetUniformLocation(shader, "u_map"), 1);
    glUniform2f(glGetUniformLocation(shader, "u_map_dims"), t->map.w, t->map.h);
    glUniform2f(glGetUniformLocation(shader, "u_atlas_dims"), t->atlas.w, t->atlas.h);

    glEnableClientState(GL_VERTEX_ARRAY);
    uint8_t indices[] = { 0,1,2, 2,3,0 };
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
}

void tilemap_destroy(struct tilemap *t)
{
    texture_destroy(&t->map.texture);
    texture_destroy(&t->atlas.texture);
    memset(t, 0, sizeof (*t));
}

#ifdef UNIT_TEST_TILEMAP
#include "libtap/tap.h"
#include "video.h"
#include "camera.h"
#include "test_video.h"

static void test_basic_output(void)
{
    struct tilemap tilemap;
    note("Testing basic shader output");
    ok(tilemap_load("t/tilemap.t-0.map", "t/tilemap.t-0.png", &tilemap));
    video_start_frame();
    position p = I*64;
    tilemap_draw(&tilemap, p);
    video_end_frame();
    ok(test_video_compare_fb_with_file("t/tilemap.t-0.640x480.png"));
    tilemap_destroy(&tilemap);
}

int main(void)
{
    video_init();
    camera_init();
    tilemap_init();
    plan(4);
    test_basic_output();
    todo();
    note("exercise file reading");
    end_todo;
    done_testing();
}
#endif
