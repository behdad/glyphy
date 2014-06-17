uniform float u_contrast;
uniform float u_gamma_adjust;
uniform float u_outline_thickness;
uniform bool  u_outline;
uniform float u_boldness;
uniform bool  u_debug;

varying vec4 v_glyph;


#define SQRT2_2 0.70710678118654757 /* 1 / sqrt(2.) */
#define SQRT2   1.4142135623730951

struct glyph_info_t {
  ivec2 nominal_size;
  ivec2 atlas_pos;
};

glyph_info_t
glyph_info_decode (vec4 v)
{
  glyph_info_t gi;
  gi.nominal_size = (ivec2 (mod (v.zw, 256.)) + 2) / 4;
  gi.atlas_pos = ivec2 (v_glyph.zw) / 256;
  return gi;
}


float
antialias (float d)
{
  return smoothstep (-.75, +.75, d);
}

vec4
source_over (const vec4 src, const vec4 dst)
{
  // http://dev.w3.org/fxtf/compositing-1/#porterduffcompositingoperators_srcover
  float alpha = src.a + (dst.a * (1. - src.a));
  return vec4 (((src.rgb * src.a) + (dst.rgb * dst.a * (1. - src.a))) / alpha, alpha);
}

void
main()
{
  vec2 p = v_glyph.xy;
  glyph_info_t gi = glyph_info_decode (v_glyph);

  /* isotropic antialiasing */
  vec2 dpdx = dFdx (p);
  vec2 dpdy = dFdy (p);
  float m = length (vec2 (length (dpdx), length (dpdy))) * SQRT2_2;

  vec4 color = vec4 (0,0,0,1);

  float gsdist = glyphy_sdf (p, gi.nominal_size GLYPHY_DEMO_EXTRA_ARGS);
  float sdist = gsdist / m * u_contrast;

  if (!u_debug) {
    sdist -= u_boldness * 10.;
    if (u_outline)
      sdist = abs (sdist) - u_outline_thickness * .5;
    if (sdist > 1.)
      discard;
    float alpha = antialias (-sdist);
    if (u_gamma_adjust != 1.)
      alpha = pow (alpha, 1./u_gamma_adjust);
    color = vec4 (color.rgb,color.a * alpha);
  } else {
    float gudist = abs (gsdist);
    float debug_color = 0.4;
    // Color the distance field red inside and green outside
    if (!glyphy_isinf (gudist))
      color = source_over (vec4 (debug_color * smoothstep (1., -1., sdist), debug_color * smoothstep (-1., 1., sdist), 0, 1. - gudist), color);

    glyphy_arc_list_t arc_list = glyphy_arc_list (p, gi.nominal_size GLYPHY_DEMO_EXTRA_ARGS);
    // Color the number of endpoints per cell blue
    color = source_over (vec4 (0, 0, debug_color, float(arc_list.num_endpoints) / float(GLYPHY_MAX_NUM_ENDPOINTS)), color);

    float pdist = glyphy_point_dist (p, gi.nominal_size GLYPHY_DEMO_EXTRA_ARGS);
    // Color points yellow
    color = source_over (vec4 (1, 1, 0, smoothstep (.06, .05, pdist)), color);
  }

  gl_FragColor = color;
}
