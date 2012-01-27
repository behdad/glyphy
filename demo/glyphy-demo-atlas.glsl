#define GLYPHY_TEXTURE1D_EXTRA_DECLS , sampler2D _tex, vec4 _atlas_info, vec4 _atlas_pos
#define GLYPHY_TEXTURE1D_EXTRA_ARGS , _tex, _atlas_info, _atlas_pos

vec4
glyphy_texture1D_func (int offset GLYPHY_TEXTURE1D_EXTRA_DECLS)
{
  vec2 orig = _atlas_pos.xy;
  int glyph_width = int (_atlas_info.z);
  vec2 pos = (_atlas_pos.xy +
	      vec2 (mod (offset, glyph_width), offset / glyph_width) +
	      vec2 (.5, .5)) / _atlas_info.xy;
  return texture2D (_tex, pos);
}
