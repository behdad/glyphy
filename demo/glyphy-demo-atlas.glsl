vec4
glyphy_texture1D_func (sampler2D tex, vec4 atlas_info, vec4 atlas_pos, int offset)
{
  vec2 orig = atlas_pos.xy;
  int glyph_width = int (atlas_info.z);
  vec2 pos = (atlas_pos.xy +
	      vec2 (mod (offset, glyph_width), offset / glyph_width) +
	      vec2 (.5, .5)) / atlas_info.xy;
  return texture2D (tex, pos);
}
