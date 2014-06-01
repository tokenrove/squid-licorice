varying vec2 v_texcoord;
uniform sampler2D u_font;
uniform vec4 u_color;

void main() {
    gl_FragColor = u_color * texture2D(u_font, v_texcoord);
}
