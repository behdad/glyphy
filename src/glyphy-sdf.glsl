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

float
glyphy(sdf) (vec2 p, sampler2D atlas_tex, vec4 atlas_info, vec4 atlas_pos, int glyph_layout)
{
  ivec2 grid_size = ivec2 (mod (glyph_layout, 256), glyph_layout / 256); // width, height
  ivec2 p_cell = ivec2 (clamp (p, vec2 (0,0), vec2(1,1) * (1.-GLYPHY_EPSILON)) * vec2 (grid_size));

  vec4 arclist_data = glyphy(texture1D_func) (atlas_tex, atlas_info, atlas_pos, p_cell.y * grid_size.x + p_cell.x);
  ivec3 arclist = glyphy(arclist_decode) (arclist_data);
  int offset = arclist.x;
  int num_endpoints =  arclist.y;
  float side = arclist.z;

  int i;
  float min_dist = GLYPHY_INFINITY;
  float min_extended_dist = GLYPHY_INFINITY;
  float min_point_dist = GLYPHY_INFINITY;

  struct {
    vec2 p0;
    vec2 p1;
    float d;
  } closest_arc;

  vec3 arc_prev = glyphy(arc_decode) (glyphy(texture1D_func) (atlas_tex, atlas_info, atlas_pos, offset));
  for (i = 1; i < num_endpoints; i++)
  {
    vec3 arc = glyphy(arc_decode) (glyphy(texture1D_func) (atlas_tex, atlas_info, atlas_pos, i + offset));
    vec2 p0 = arc_prev.rg;
    arc_prev = arc;
    vec2 p1 = arc.rg;
    float d = arc.b;

    if (glyphy(isinf) (d)) continue;

    // for highlighting points
    min_point_dist = min (min_point_dist, distance (p, p1));

    // unsigned distance
    float d2 = glyphy(tan2atan) (d);
    if (dot (p - p0, (p1 - p0) * mat2(1,  d2, -d2, 1)) >= 0 &&
	dot (p - p1, (p1 - p0) * mat2(1, -d2,  d2, 1)) <= 0)
    {
      vec2 c = glyphy(arc_center) (p0, p1, d);
      float signed_dist = (distance (p, c) - distance (p0, c));
      float dist = abs (signed_dist);
      if (dist <= min_dist) {
	min_dist = dist;
	side = sign (d) * (signed_dist >= 0 ? -1 : +1);
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
	float new_extended_dist = glyphy(arc_extended_dist) (p, p0, p1, d);
	float old_extended_dist = glyphy(arc_extended_dist) (p, closest_arc.p0, closest_arc.p1, closest_arc.d);

	float extended_dist = abs (new_extended_dist) <= abs (old_extended_dist) ?
			      old_extended_dist : new_extended_dist;

	/* For emboldening and stuff: */
	// min_dist = abs (extended_dist);
	side = sign (extended_dist);
      }
    }
  }

  if (side == 0) {
    // Technically speaking this should not happen, but it does.  So fix it.
    float extended_dist = glyphy(arc_extended_dist) (p, closest_arc.p0, closest_arc.p1, closest_arc.d);
    side = sign (extended_dist);
  }

  return min_dist * side;
}
