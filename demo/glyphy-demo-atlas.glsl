uniform sampler2D u_atlas_tex;
uniform vec3 u_atlas_info;

#define GLYPHY_TEXTURE1D_EXTRA_DECLS , sampler2D _tex, vec3 _atlas_info, vec2 _atlas_pos
#define GLYPHY_TEXTURE1D_EXTRA_ARGS , _tex, _atlas_info, _atlas_pos
#define GLYPHY_DEMO_EXTRA_ARGS , u_atlas_tex, u_atlas_info, f_atlas_pos

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
