uniform mat4 u_matViewProjection;

attribute vec4 a_glyph_vertex;

varying vec4 v_glyph;

vec4
glyph_vertex_transcode (vec2 v)
{
  ivec2 g = ivec2 (v);
  return vec4 (mod (g, 2), g / 2);
}

void
main()
{
  gl_Position = u_matViewProjection * vec4 (a_glyph_vertex.xy, 0, 1);
  v_glyph = glyph_vertex_transcode (a_glyph_vertex.zw);
}
