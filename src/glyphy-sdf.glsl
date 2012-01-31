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

#ifndef GLYPHY_TEXTURE1D_FUNC
#define GLYPHY_TEXTURE1D_FUNC glyphy_texture1D_func
#endif
#ifndef GLYPHY_TEXTURE1D_EXTRA_DECLS
#define GLYPHY_TEXTURE1D_EXTRA_DECLS
#endif
#ifndef GLYPHY_TEXTURE1D_EXTRA_ARGS
#define GLYPHY_TEXTURE1D_EXTRA_ARGS
#endif

#ifndef GLYPHY_SDF_TEXTURE1D_FUNC
#define GLYPHY_SDF_TEXTURE1D_FUNC GLYPHY_TEXTURE1D_FUNC
#endif
#ifndef GLYPHY_SDF_TEXTURE1D_EXTRA_DECLS
#define GLYPHY_SDF_TEXTURE1D_EXTRA_DECLS GLYPHY_TEXTURE1D_EXTRA_DECLS
#endif
#ifndef GLYPHY_SDF_TEXTURE1D_EXTRA_ARGS
#define GLYPHY_SDF_TEXTURE1D_EXTRA_ARGS GLYPHY_TEXTURE1D_EXTRA_ARGS
#endif
#ifndef GLYPHY_SDF_TEXTURE1D(offset)
#define GLYPHY_SDF_TEXTURE1D(offset) \
	GLYPHY_SDF_TEXTURE1D_FUNC (offset GLYPHY_TEXTURE1D_EXTRA_ARGS)
#endif

glyphy_arc_list_t
glyphy_arc_list (vec2 p, int glyph_layout GLYPHY_SDF_TEXTURE1D_EXTRA_DECLS)
{
  int cell_offset = glyphy_arc_list_offset (p, glyph_layout);
  vec4 arc_list_data = GLYPHY_SDF_TEXTURE1D (cell_offset);
  return glyphy_arc_list_decode (arc_list_data);
}

float
glyphy_sdf (vec2 p, int glyph_layout GLYPHY_SDF_TEXTURE1D_EXTRA_DECLS)
{
  glyphy_arc_list_t arc_list = glyphy_arc_list (p, glyph_layout  GLYPHY_SDF_TEXTURE1D_EXTRA_ARGS);

  /* Short-circuits */
  if (arc_list.num_endpoints == 0) {
    /* far-away cell */
    return GLYPHY_INFINITY * arc_list.side;
  } if (arc_list.num_endpoints == -1) {
    /* single-line */
    float angle = arc_list.line_angle;
    vec2 n = vec2 (cos (angle), sin (angle));
    return dot (p - vec2(.5,.5), n) - arc_list.line_distance;
  }

  float side = arc_list.side;
  float min_dist = GLYPHY_INFINITY;
  glyphy_arc_t closest_arc;

  glyphy_arc_endpoint_t endpoint_prev, endpoint;
  endpoint_prev = glyphy_arc_endpoint_decode (GLYPHY_SDF_TEXTURE1D (arc_list.offset));
  for (int i = 1; i < arc_list.num_endpoints; i++)
  {
    endpoint = glyphy_arc_endpoint_decode (GLYPHY_SDF_TEXTURE1D (arc_list.offset + i));
    glyphy_arc_t a = glyphy_arc_t (endpoint_prev.p, endpoint.p, endpoint.d);
    endpoint_prev = endpoint;
    if (glyphy_isinf (a.d)) continue;

    if (glyphy_arc_wedge_contains (a, p))
    {
      float sdist = glyphy_arc_wedge_signed_dist (a, p);
      float udist = abs (sdist) - GLYPHY_EPSILON;
      if (udist <= min_dist) {
	min_dist = udist;
	side = sdist <= 0 ? -1 : +1;
      }
    } else {
      float udist = min (distance (p, a.p0), distance (p, a.p1));
      if (udist < min_dist) {
	min_dist = udist;
	side = 0; /* unsure */
	closest_arc = a;
      } else if (side == 0 && udist == min_dist) {
	/* If this new distance is the same as the current minimum,
	 * compare extended distances.  Take the sign from the arc
	 * with larger extended distance. */
	float old_ext_dist = glyphy_arc_extended_dist (closest_arc, p);
	float new_ext_dist = glyphy_arc_extended_dist (a, p);

	float ext_dist = abs (new_ext_dist) <= abs (old_ext_dist) ?
			 old_ext_dist : new_ext_dist;

	/* For emboldening and stuff: */
	// min_dist = abs (ext_dist);
	side = sign (ext_dist);
      }
    }
  }

  if (side == 0) {
    // Technically speaking this should not happen, but it does.  So try to fix it.
    float ext_dist = glyphy_arc_extended_dist (closest_arc, p);
    side = sign (ext_dist);
  }

  return min_dist * side;
}

float
glyphy_point_dist (vec2 p, int glyph_layout GLYPHY_SDF_TEXTURE1D_EXTRA_DECLS)
{
  glyphy_arc_list_t arc_list = glyphy_arc_list (p, glyph_layout  GLYPHY_SDF_TEXTURE1D_EXTRA_ARGS);

  float side = arc_list.side;
  float min_dist = GLYPHY_INFINITY;

  if (arc_list.num_endpoints == 0)
    return min_dist;

  glyphy_arc_endpoint_t endpoint_prev, endpoint;
  endpoint_prev = glyphy_arc_endpoint_decode (GLYPHY_SDF_TEXTURE1D (arc_list.offset));
  for (int i = 1; i < arc_list.num_endpoints; i++)
  {
    endpoint = glyphy_arc_endpoint_decode (GLYPHY_SDF_TEXTURE1D (arc_list.offset + i));
    if (glyphy_isinf (endpoint.d)) continue;
    min_dist = min (min_dist, distance (p, endpoint.p));
  }
  return min_dist;
}
