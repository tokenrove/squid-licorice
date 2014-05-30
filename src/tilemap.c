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
static const GLchar tilemap_vertex_shader_src[] = " \
attribute vec2 a_vertex;                                           \
uniform mat4 u_projection;                                         \
uniform mat4 u_modelview;                                          \
varying vec2 v_texcoord;                                           \
                                                                   \
void main()                                                        \
{                                                                  \
  gl_Position = u_projection * u_modelview * vec4(a_vertex, 0., 1.);    \
  v_texcoord = a_vertex;                                         \
}";

static const GLchar tilemap_fragment_shader_src[] = "\
varying vec2 v_texcoord;                                \
uniform sampler2D u_atlas;                              \
uniform sampler2D u_map;                                \
uniform vec4 u_map_atlas_dims;                          \
                                                        \
void main() {                                           \
  vec2 map_coordinate = vec2(v_texcoord.x/u_map_atlas_dims.x, v_texcoord.y/u_map_atlas_dims.y);     \
  float tile_idx = 255. * texture2D(u_map, map_coordinate).r;           \
  vec2 atlas_coordinate = mod(v_texcoord, u_map_atlas_dims.z);                         \
  atlas_coordinate.y += u_map_atlas_dims.z * tile_idx; /* XXX technically this is tile height, not width */ \
  gl_FragColor = texture2D(u_atlas, vec2(atlas_coordinate.x/u_map_atlas_dims.z, atlas_coordinate.y/u_map_atlas_dims.w)); \
}                                                       \
";

void tilemap_init(void)
{
    texture_init();
}

bool tilemap_load(char *map_path, char *atlas_path, struct tilemap *t)
{
    *t = (struct tilemap){0};

    // reset pixel store: can we use a pack/unpack alignment of 4 here?
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

    // load map
    FILE *fp = fopen(map_path, "r");
    ENSURE(fp);
    uint8_t buffer[4];
    ENSURE(4 == fread(buffer, 1, 4, fp));
    for (int i = 0; i < 4; ++i) ENSURE(buffer[i] == "MAP\x01"[i]);
    struct {
        uint16_t width, height;
        uint8_t *map;
    } map;
    ENSURE(1 == fread(&map.width, 2, 1, fp));
    map.width = ntohs(map.width);
    ENSURE(1 == fread(&map.height, 2, 1, fp));
    map.height = ntohs(map.height);
    size_t n = map.width*map.height;
    map.map = malloc(n);
    ENSURE(map.map);
    ENSURE(fread(map.map, 1, n, fp) == n);
    fclose(fp);
    glGenTextures(1, &t->map_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, t->map_texture);
    // XXX needs to be power of 2? what about alignment by 4s?
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, map.width, map.height, 0, GL_RED, GL_UNSIGNED_BYTE, map.map);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    free(map.map);

    t->atlas_texture = texture_from_png(atlas_path);
    ENSURE(-1 != t->atlas_texture.id);

    t->shader = build_shader_program("tilemap", tilemap_vertex_shader_src, tilemap_fragment_shader_src);
    if (0 == t->shader) return false; /* XXX handle this error better */

    // XXX urk, obviously not fixed vertices
    const GLfloat vertices[] = { 0., 0., 720., 0., 720., 240., 0., 240. };
    glGenBuffers(1, &t->a_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, t->a_vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_DYNAMIC_DRAW);

    t->dims[0] = (float)map.width*t->atlas_texture.width;
    t->dims[1] = (float)map.height*t->atlas_texture.width; // tile height, but we assume it's the same, here
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
    glBindTexture(GL_TEXTURE_2D, t->map_texture);

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
    glDeleteTextures(1, &t->map_texture);
    glDeleteTextures(1, &t->atlas_texture.id);
    glDeleteBuffers(1, &t->a_vertices);
    glDeleteProgram(t->shader);
    *t = (struct tilemap){0};
}
