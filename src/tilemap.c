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
 * We pass in the map and atlas dimensions in u_map_atlas_dims, and we
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
static GLuint shader;

void tilemap_init(void)
{
    texture_init();
    if (0 == shader)
        shader = build_shader_program("tilemap", tilemap_vertex_shader_src, tilemap_fragment_shader_src);
    ENSURE(shader);
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
    ENSURE(texture_from_pels(&t->map_texture, data, width, height, 1));
    free(data);
}

bool tilemap_load(const char *map_path, const char *atlas_path, struct tilemap *t)
{
    *t = (struct tilemap){0};

    load_map(t, map_path);
    ENSURE(texture_from_png(&t->atlas_texture, atlas_path));

    uint16_t tile_width = t->atlas_texture.width;
    uint16_t tile_height = tile_width; // XXX we assume it's the same, here
    t->dims[0] = (float)t->map_texture.width*tile_width;
    t->dims[1] = (float)t->map_texture.height*tile_height;
    t->dims[2] = (float)t->atlas_texture.width;
    t->dims[3] = (float)t->atlas_texture.height;

    const GLfloat vertices[] = { 0., 0., t->dims[0], 0., t->dims[0], t->dims[1], 0., t->dims[1] };
    glGenBuffers(1, &t->a_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, t->a_vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_DYNAMIC_DRAW);

    return true;
}

void tilemap_draw(struct tilemap *t, position offset)
{
    ENSURE(shader);
    glUseProgram(shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t->atlas_texture.id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, t->map_texture.id);

    glBindBuffer(GL_ARRAY_BUFFER, t->a_vertices);
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
    glUniform4fv(glGetUniformLocation(shader, "u_map_atlas_dims"), 1, t->dims);

    glEnableClientState(GL_VERTEX_ARRAY);
    uint8_t indices[] = { 0,1,2, 2,3,0 };
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
}

void tilemap_destroy(struct tilemap *t)
{
    texture_destroy(&t->map_texture);
    texture_destroy(&t->atlas_texture);
    glDeleteBuffers(1, &t->a_vertices);
    *t = (struct tilemap){0};
}
