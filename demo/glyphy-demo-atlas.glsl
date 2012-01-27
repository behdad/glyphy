uniform ivec3 u_texSize;

int mod (const int a, const int b) { return a - (a / b) * b; }
int div (const int a, const int b) { return a / b; }

vec4
glyphy_texture1D_func (sampler2D tex, vec4 atlas_info, int offset)
{
  vec2 orig = atlas_info.xy;
  return texture2D (tex, vec2 ((orig.x + mod (offset, u_texSize.z) + .5) / float (u_texSize.x),
			       (orig.y + div (offset, u_texSize.z) + .5) / float (u_texSize.y)));
}
