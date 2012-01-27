int mod (const int a, const int b) { return a - (a / b) * b; }
int div (const int a, const int b) { return a / b; }

vec4
glyphy_texture1D_func (sampler2D tex, vec4 atlas_info, vec4 atlas_pos, int offset)
{
  vec2 orig = atlas_pos.xy;
  ivec4 atlas = ivec4 (atlas_info);
  return texture2D (tex, vec2 ((orig.x + mod (offset, atlas.z) + .5) / float (atlas.x),
			       (orig.y + div (offset, atlas.z) + .5) / float (atlas.y)));
}
