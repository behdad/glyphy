uniform mat4 u_matViewProjection;

attribute vec4 a_position;
attribute vec2 a_glyph;

varying vec4 v_glyph;

vec4 glyph_decode_v (vec2 v)
{
  ivec2 g = ivec2 (v);
  return vec4 (mod (g, 2), g / 2);
}

void main()
{
  gl_Position = u_matViewProjection * a_position;
  v_glyph = glyph_decode_v (a_glyph);
}
