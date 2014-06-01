#include <stdio.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include <arpa/inet.h>

#include "ensure.h"
#include "shader.h"
#include "tilemap.h"

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

    t->shader = build_shader_program("tilemap", tilemap_vertex_shader_src, tilemap_fragment_shader_src);
    if (0 == t->shader) return false; /* XXX handle this error better */

    // XXX urk, obviously not fixed vertices
    const GLfloat vertices[] = { 0., 0., 720., 0., 720., 240., 0., 240. };
    glGenBuffers(1, &t->a_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, t->a_vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_DYNAMIC_DRAW);

    uint16_t tile_width = t->atlas_texture.width;
    uint16_t tile_height = tile_width; // XXX we assume it's the same, here
    t->dims[0] = (float)t->map_texture.width*tile_width;
    t->dims[1] = (float)t->map_texture.height*tile_height;
    t->dims[2] = (float)t->atlas_texture.width;
    t->dims[3] = (float)t->atlas_texture.height;

    return true;
}

void tilemap_draw(struct tilemap *t, float *mv_matrix, float *projection_matrix)
{
    uint8_t indices[] = { 0,1,2, 2,3,0 };

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t->atlas_texture.id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, t->map_texture.id);

    glUseProgram(t->shader);

    glBindBuffer(GL_ARRAY_BUFFER, t->a_vertices);
    GLint vertices_atloc = glGetAttribLocation(t->shader, "a_vertex");
    glVertexAttribPointer(vertices_atloc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertices_atloc);

    GLuint mv_uniloc = glGetUniformLocation(t->shader, "u_modelview");
    glUniformMatrix4fv(mv_uniloc, 1, GL_FALSE, mv_matrix);
    GLuint projection_uniloc = glGetUniformLocation(t->shader, "u_projection");
    glUniformMatrix4fv(projection_uniloc, 1, GL_FALSE, projection_matrix);

    GLuint atlas_uniloc = glGetUniformLocation(t->shader, "u_atlas");
    glUniform1i(atlas_uniloc, 0);
    GLuint map_uniloc = glGetUniformLocation(t->shader, "u_map");
    glUniform1i(map_uniloc, 1);
    GLuint map_dims_uniloc = glGetUniformLocation(t->shader, "u_map_atlas_dims");
    glUniform4fv(map_dims_uniloc, 1, t->dims);

    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
}

void tilemap_destroy(struct tilemap *t)
{
    texture_destroy(&t->map_texture);
    texture_destroy(&t->atlas_texture);
    glDeleteBuffers(1, &t->a_vertices);
    glDeleteProgram(t->shader);
    *t = (struct tilemap){0};
}
