void main()
{
  vec2 p = v_glyph.xy;
  ivec2 glyph_offset = ivec2 (int (v_glyph.z), int (v_glyph.w));

  /* isotropic antialiasing */
  float m = length (vec2 (float (dFdx (p)),
			  float (dFdy (p))));
  //float m = float (fwidth (p)); //for broken dFdx/dFdy

  float sdist = glyphy_sdf (p, glyph_offset);
  float udist = abs (sdist);

  vec4 color = vec4(0,0,0,1);

  // Color the outline red
  color += vec4(1,0,0,0) * smoothstep (2 * m, 0, udist);

  // Color the distance field in green
  color += vec4(0,1,0,0) * ((1 + sin (sdist / m))) * sin (pow (udist, .8) * 3.14159265358979) * .5;

/*
  // Color points green
  color = mix(vec4(0,1,0,1), color, smoothstep (2 * m, 3 * m, min_point_dist));

  // Color the number of endpoints per cell blue
  color += vec4(0,0,1,0) * num_endpoints * 16./255.;
*/

  // Color the inside of the glyph a light red
  color += vec4(.5,0,0,0) * smoothstep (m, -m, sdist);

//  color = vec4(1,1,1,1) * smoothstep (-m, m, sdist);

  gl_FragColor = color;
}
