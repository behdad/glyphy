#ifdef GL_ES
precision highp float;
precision highp int;
#endif

uniform float u_contrast;
uniform float u_gamma_adjust;
uniform bool  u_debug;

varying vec4 v_glyph;

struct glyph_info_t {
  ivec2 nominal_size;
  ivec2 atlas_pos;
};

glyph_info_t
glyph_info_decode (vec4 v)
{
  glyph_info_t gi;
  gi.nominal_size = ivec2 (mod (v.zw, 256));
  gi.atlas_pos = ivec2 (v_glyph.zw) / 256;
  return gi;
}

void
main()
{
  vec2 p = v_glyph.xy;
  glyph_info_t gi = glyph_info_decode (v_glyph);

  /* isotropic antialiasing */
  vec2 dpdx = dFdx (p);
  vec2 dpdy = dFdy (p);
  float m = length (vec2 (length (dpdx), length (dpdy)));

  vec4 color = vec4 (0,0,0,1);

  float gsdist = glyphy_sdf (p, gi.nominal_size GLYPHY_DEMO_EXTRA_ARGS);
  float sdist = gsdist / m * u_contrast;

  if (!u_debug) {
    if (sdist > 1)
      discard;
    float alpha = smoothstep (1, -1, sdist);
    if (u_gamma_adjust != 1)
      alpha = pow (alpha, 1./u_gamma_adjust);
    color = vec4 (color.rgb,color.a * alpha);
  } else {
    color = vec4 (0,0,0,0);

    // Color the inside of the glyph a light red
    color += vec4 (.5,0,0,.5) * smoothstep (1, -1, sdist);

    float udist = abs (sdist);
    float gudist = abs (gsdist);
    // Color the outline red
    color += vec4 (1,0,0,1) * smoothstep (2, 1, udist);
    // Color the distance field in green
    if (!glyphy_isinf (udist))
      color += vec4 (0,.3,0,(1 + sin (sdist)) * abs(1 - gsdist * 3) / 3.);

    float pdist = glyphy_point_dist (p, gi.nominal_size GLYPHY_DEMO_EXTRA_ARGS);
    // Color points green
    color = mix (vec4 (0,1,0,.5), color, smoothstep (.05, .06, pdist));

    glyphy_arc_list_t arc_list = glyphy_arc_list (p, gi.nominal_size GLYPHY_DEMO_EXTRA_ARGS);
    // Color the number of endpoints per cell blue
    color += vec4 (0,0,1,.1) * arc_list.num_endpoints * 32./255.;
  }

  gl_FragColor = color;
}
