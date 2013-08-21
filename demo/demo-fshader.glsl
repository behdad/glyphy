uniform float u_contrast;
uniform float u_gamma_adjust;
uniform float u_outline_thickness;
uniform bool  u_outline;
uniform float u_boldness;
uniform bool  u_debug;

varying vec4 v_glyph;

#ifdef GLES2
#  extension GL_OES_standard_derivatives : enable
#endif

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
antialias_axisaligned (float d)
{
  return clamp (d + .5, 0., 1.);
}

float
antialias_diagonal (float d)
{
  /* TODO optimize this */
  if (d <= -SQRT2_2) return 0.;
  if (d >= +SQRT2_2) return 1.;
  if (d <= 0.) return pow (d + SQRT2_2, 2.);
  return 1. - pow (SQRT2_2 - d, 2.);
}

float
antialias (float d, float w)
{
  /* w is 1.0 for axisaligned pixels, and SQRT2 for diagonal pixels,
   * and something in between otherwise... */
  return mix (antialias_axisaligned (d), antialias_diagonal (d), clamp ((w - 1.) / (SQRT2 - 1.), 0., 1.));
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
  float w = abs (normalize (dpdx).x) + abs (normalize (dpdy).x);

  vec4 color = vec4 (0,0,0,1);

  float gsdist = glyphy_sdf (p, gi.nominal_size GLYPHY_DEMO_EXTRA_ARGS);
  float sdist = gsdist / m * u_contrast;

  if (!u_debug) {
    sdist -= u_boldness * 10.;
    if (u_outline)
      sdist = abs (sdist) - u_outline_thickness * .5;
    if (sdist > 1.)
      discard;
    float alpha = antialias (-sdist, w);
    if (u_gamma_adjust != 1.)
      alpha = pow (alpha, 1./u_gamma_adjust);
    color = vec4 (color.rgb,color.a * alpha);
  } else {
    color = vec4 (0,0,0,0);

    // Color the inside of the glyph a light red
    color += vec4 (.5,0,0,.5) * smoothstep (1., -1., sdist);

    float udist = abs (sdist);
    float gudist = abs (gsdist);
    // Color the outline red
    color += vec4 (1,0,0,1) * smoothstep (2., 1., udist);
    // Color the distance field in green
    if (!glyphy_isinf (udist))
      color += vec4 (0,.3,0,(1. + sin (sdist)) * abs(1. - gsdist * 3.) / 3.);

    float pdist = glyphy_point_dist (p, gi.nominal_size GLYPHY_DEMO_EXTRA_ARGS);
    // Color points green
    color = mix (vec4 (0,1,0,.5), color, smoothstep (.05, .06, pdist));

    glyphy_arc_list_t arc_list = glyphy_arc_list (p, gi.nominal_size GLYPHY_DEMO_EXTRA_ARGS);
    // Color the number of endpoints per cell blue
    color += vec4 (0,0,1,.1) * float(arc_list.num_endpoints) * 32./255.;
  }

  gl_FragColor = color;
}
