varying vec2 v_texcoord;
uniform sampler2D u_font;
uniform vec4 u_color;

void main() {
    vec4 c = texture2D(u_font, v_texcoord);
    if (c.a == 0.) discard;
    gl_FragColor = c * u_color;
}
