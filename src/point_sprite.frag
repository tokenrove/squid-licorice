#version 120
uniform vec2 u_texcoord;
uniform vec2 u_ratio;
uniform sampler2D u_atlas;

void main() {
    vec2 pc = gl_PointCoord * u_ratio;
    vec4 c = texture2D(u_atlas, u_texcoord + pc);
    if (c.a == 0.) discard;
    gl_FragColor = c;
}
