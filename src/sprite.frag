varying vec2 v_texcoord;
uniform sampler2D u_atlas;

void main() {
    vec4 c = texture2D(u_atlas, v_texcoord);
    if (c.a == 0.) discard;
    gl_FragColor = c;
}
