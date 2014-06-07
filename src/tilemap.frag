varying vec2 v_texcoord;
uniform sampler2D u_map, u_atlas;
uniform vec2 u_map_dims, u_atlas_dims;

void main() {
  vec2 map_coordinate = vec2(v_texcoord.x/u_map_dims.x, v_texcoord.y/u_map_dims.y);
  float tile_idx = 255. * texture2D(u_map, map_coordinate).r;
  vec2 atlas_coordinate = mod(v_texcoord, u_atlas_dims.x);
  atlas_coordinate.y += u_atlas_dims.x * tile_idx; /* XXX technically this is tile height, not width */
  gl_FragColor = texture2D(u_atlas, vec2(atlas_coordinate.x/u_atlas_dims.x, atlas_coordinate.y/u_atlas_dims.y));
}
