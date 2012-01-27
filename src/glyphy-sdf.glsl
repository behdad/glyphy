/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Google Author(s): Behdad Esfahbod, Maysum Panju
 */

vec4
/*<*/glyphy_fragment_color/*>*/ (vec2 p, vec4 v_glyph)
{
  ivec2 glyph_offset = ivec2(int(v_glyph.z), int(v_glyph.w));

  /* isotropic antialiasing */
  float m = length (vec2 (float (dFdx (p)),
			  float (dFdy (p))));
  //float m = float (fwidth (p)); //for broken dFdx/dFdy

  int p_cell_x = int (clamp (p.x, 0., 1.-1e-5) * 16 /*GRID_W*/);
  int p_cell_y = int (clamp (p.y, 0., 1.-1e-5) * 16 /*GRID_H*/);

  vec4 arclist_data = tex_1D (glyph_offset, p_cell_y * 16 /*GRID_W*/ + p_cell_x);
  ivec3 arclist = /*<*/glyphy_arclist_decode/*>*/ (arclist_data);
  int offset = arclist.x;
  int num_endpoints =  arclist.y;
  int side = arclist.z;

  int i;
  float min_dist = 1.5;
  float min_extended_dist = 1.5;
  float min_point_dist = 1.5;

  struct {
    vec2 p0;
    vec2 p1;
    float d;
  } closest_arc;

  vec4 result = vec4(0,0,0,1);

  vec3 arc_prev = /*<*/glyphy_arc_decode/*>*/ (tex_1D (glyph_offset, offset));
  for (i = 1; i < num_endpoints; i++)
  {
    vec3 arc = /*<*/glyphy_arc_decode/*>*/ (tex_1D (glyph_offset, i + offset));
    vec2 p0 = arc_prev.rg;
    arc_prev = arc;
    float d = arc.b;
    vec2 p1 = arc.rg;

    if (d == -.5 /*MAX_D*/) continue;

    // for highlighting points
    min_point_dist = min (min_point_dist, distance (p, p1));

    // unsigned distance
    float d2 = /*<*/glyphy_tan2atan/*>*/ (d);
    if (dot (p - p0, (p1 - p0) * mat2(1,  d2, -d2, 1)) >= 0 &&
	dot (p - p1, (p1 - p0) * mat2(1, -d2,  d2, 1)) <= 0)
    {
      vec2 c = /*<*/glyphy_arc_center/*>*/ (p0, p1, d);
      float signed_dist = (distance (p, c) - distance (p0, c));
      float dist = abs (signed_dist);
      if (dist <= min_dist) {
	min_dist = dist;
	side = ((sign (d) >= 0) ? +1 : -1) * (sign (signed_dist) >= 0 ? -1 : +1);
      }
    } else {
      float dist = min (distance (p, p0), distance (p, p1));
      if (dist < min_dist) {
	min_dist = dist;
	side = 0; /* unsure */
	closest_arc.p0 = p0;
	closest_arc.p1 = p1;
	closest_arc.d  = d;
      } else if (dist == min_dist && side == 0) {
	// If this new distance is the same as the current minimum, compare extended distances.
	// Take the sign from the arc with larger extended distance.
	float new_extended_dist = /*<*/glyphy_arc_extended_dist/*>*/ (p, p0, p1, d);
	float old_extended_dist = /*<*/glyphy_arc_extended_dist/*>*/ (p, closest_arc.p0, closest_arc.p1, closest_arc.d);

	float extended_dist = abs (new_extended_dist) <= abs (old_extended_dist) ?
			      old_extended_dist : new_extended_dist;

//	      min_dist = abs (extended_dist);
	side = extended_dist < 0 ? -1 : +1;
      }
    }
  }

  if (side == 0)
  {
    // Technically speaking this should not happen, but it does.  So fix it.
    float extended_dist = /*<*/glyphy_arc_extended_dist/*>*/ (p, closest_arc.p0, closest_arc.p1, closest_arc.d);
    side = extended_dist < 0 ? -1 : +1;
    //result = vec4 (1,0,0,0);
    //return result;
  }

  float abs_dist = min_dist;
  min_dist *= side;


/*
  // Color the outline red
  result += vec4(1,0,0,0) * smoothstep (2 * m, 0, abs_dist);

  // Color the distance field in green
  result += vec4(0,1,0,0) * ((1 + sin (min_dist / m))) * sin (pow (abs_dist, .8) * 3.14159265358979) * .5;

  // Color points green
  result = mix(vec4(0,1,0,1), result, smoothstep (2 * m, 3 * m, min_point_dist));

  // Color the number of endpoints per cell blue
  result += vec4(0,0,1,0) * num_endpoints * 16./255.;

  // Color the inside of the glyph a light red
  result += vec4(.5,0,0,0) * smoothstep (m, -m, min_dist);
*/
  result = vec4(1,1,1,1) * smoothstep (-m, m, min_dist);
  return result;
}

