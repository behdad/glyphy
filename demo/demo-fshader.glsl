in vec2 v_texcoord;
flat in vec4 v_bandTransform;
flat in ivec4 v_glyphData;

out vec4 fragColor;

void main ()
{
  ivec2 glyphLoc = v_glyphData.xy;
  int numHBands = v_glyphData.z;
  int numVBands = v_glyphData.w;

  float coverage = glyphy_slug_render (v_texcoord, v_bandTransform,
				       glyphLoc, numHBands, numVBands);

  fragColor = vec4 (0.0, 0.0, 0.0, coverage);
}
