uniform mat4 u_matViewProjection;
attribute vec4 a_position;
attribute vec2 a_glyph;
varying vec4 v_glyph;

int mod (const int a, const int b) { return a - (a / b) * b; }
int div (const int a, const int b) { return a / b; }

vec4 glyph_decode (vec2 v)
{
  ivec2 g = ivec2 (int(v.x), int(v.y));
  return vec4 (mod (g.x, 2), mod (g.y, 2), div (g.x, 2), div(g.y, 2));
}

void main()
{
  gl_Position = u_matViewProjection * a_position;
  v_glyph = glyph_decode (a_glyph);
}
