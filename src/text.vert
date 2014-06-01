attribute vec2 a_vertex;
uniform mat4 u_projection;
uniform mat4 u_modelview;
uniform vec2 u_offset;
uniform vec2 u_font_dims;
varying vec2 v_texcoord;

void main()
{
    gl_Position = u_projection * u_modelview * vec4(a_vertex, 0., 1.);
    v_texcoord = a_vertex + u_offset;
    v_texcoord.x /= u_font_dims.x;
    v_texcoord.y /= u_font_dims.y;
}
