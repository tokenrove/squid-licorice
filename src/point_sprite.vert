attribute vec2 a_point;
uniform mat4 u_projection;
uniform vec2 u_translation;

void main()
{
    mat4 T = mat4(1.0, 0.0, 0.0, u_translation.x,
                  0.0, 1.0, 0.0, u_translation.y,
                  0.0, 0.0, 1.0, 0.0,
                  0.0, 0.0, 0.0, 1.0);

    gl_Position = u_projection * (vec4(a_point, 0., 1.) * T);
}
