uniform sampler2D u_tex;
uniform ivec3 u_texSize;
varying vec4 v_glyph;

int mod (const int a, const int b) { return a - (a / b) * b; }
int div (const int a, const int b) { return a / b; }
vec4 tex_1D (ivec2 offset, int i)
{
  vec2 orig = offset;
  return texture2D (u_tex, vec2 ((orig.x + mod (i, u_texSize.z) + .5) / float (u_texSize.x),
				 (orig.y + div (i, u_texSize.z) + .5) / float (u_texSize.y)));
}
