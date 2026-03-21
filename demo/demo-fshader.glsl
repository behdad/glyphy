in vec2 v_texcoord;
flat in ivec4 v_glyphData;

out vec4 fragColor;

void main ()
{
  int glyphLoc = v_glyphData.x;

  float coverage = glyphy_slug_render (v_texcoord, glyphLoc);

  fragColor = vec4 (0.0, 0.0, 0.0, coverage);
}
