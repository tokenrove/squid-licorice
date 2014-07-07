varying vec2 v_texcoord;
uniform sampler2D u_atlas;
uniform bool u_all_white;

void main() {
    vec4 c = texture2D(u_atlas, v_texcoord);
    if (c.a == 0.) discard;
    gl_FragColor = u_all_white ? vec4(1.,1.,1.,c.a) : c;
}
