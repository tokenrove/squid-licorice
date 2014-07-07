attribute vec2 a_vertex;
uniform mat4 u_projection;
uniform vec2 u_translation;
uniform float u_scaling, u_rotation;
varying vec2 v_texcoord;

void main()
{
    mat4 R = mat4(cos(u_rotation), -sin(u_rotation), 0.0, 0.0,
                  sin(u_rotation), cos(u_rotation), 0.0, 0.0,
                  0.0, 0.0, 1.0, 0.0,
                  0.0, 0.0, 0.0, 1.0);
    mat4 T = mat4(1.0, 0.0, 0.0, u_translation.x,
                  0.0, 1.0, 0.0, u_translation.y,
                  0.0, 0.0, 1.0, 0.0,
                  0.0, 0.0, 0.0, 1.0);
    mat4 S = mat4(u_scaling, 0.0, 0.0, 0.0,
                  0.0, u_scaling, 0.0, 0.0,
                  0.0, 0.0, 1.0, 0.0,
                  0.0, 0.0, 0.0, 1.0);

    gl_Position = u_projection * (vec4(a_vertex, 0, 1) * R * T * S);
    v_texcoord = a_vertex;
}
