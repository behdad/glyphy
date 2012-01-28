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


#ifndef GLYPHY_INFINITY
#define GLYPHY_INFINITY 1e9
#endif
#ifndef GLYPHY_EPSILON
#define GLYPHY_EPSILON  1e-5
#endif


struct glyphy_arc_t {
  vec2  p0;
  vec2  p1;
  float d;
};

struct glyphy_arc_endpoint_t {
  vec2  p;
  float d;
};

struct glyphy_arc_list_t {
  /* Offset to the arc-endpoints from the beginning of the glyph blob */
  int offset;
  /* Number of endpoints in the list (may be zero) */
  int num_endpoints;
  /* If num_endpoints is zero, this specifies whether we are inside (-1)
   * or outside (+1).  Otherwise we're unsure (0). */
  int side;
};

bool
glyphy_isinf (float v)
{
  return abs (v) > GLYPHY_INFINITY / 2.;
}

bool
glyphy_iszero (float v)
{
  return abs (v) < GLYPHY_EPSILON * 2.;
}

vec2
glyphy_perpendicular (const vec2 v)
{
  return vec2 (-v.y, v.x);
}

int
glyphy_float_to_byte (const float v)
{
  return int (v * (256 - GLYPHY_EPSILON));
}

ivec4
glyphy_vec4_to_bytes (const vec4 v)
{
  return ivec4 (v * (256 - GLYPHY_EPSILON));
}

ivec2
glyphy_float_to_two_nimbles (const float v)
{
  int f = glyphy_float_to_byte (v);
  return ivec2 (f / 16, mod (f, 16));
}

/* returns tan (2 * atan (d)) */
float
glyphy_tan2atan (float d)
{
  return 2 * d / (1 - d * d);
}

glyphy_arc_endpoint_t
glyphy_arc_endpoint_decode (const vec4 v)
{
  /* Note that this never returns d == 0.  For straight lines,
   * a d value of .0039215686 is returned.  In fact, the d has
   * that bias for all values.
   */
  vec2 p = (vec2 (glyphy_float_to_two_nimbles (v.a)) + v.gb) / 16;
  float d = v.r;
  if (d == 0)
    d = GLYPHY_INFINITY;
#define GLYPHY_MAX_D .5
    d = GLYPHY_MAX_D * (2 * d - 1);
#undef GLYPHY_MAX_D
  return glyphy_arc_endpoint_t (p, d);
}

vec2
glyphy_arc_center (glyphy_arc_t a)
{
  return mix (a.p0, a.p1, .5) +
	 glyphy_perpendicular (a.p1 - a.p0) / (2 * glyphy_tan2atan (a.d));
}

bool
glyphy_arc_wedge_contains (const glyphy_arc_t a, const vec2 p)
{
  float d2 = glyphy_tan2atan (a.d);
  return dot (p - a.p0, (a.p1 - a.p0) * mat2(1,  d2, -d2, 1)) >= 0 &&
	 dot (p - a.p1, (a.p1 - a.p0) * mat2(1, -d2,  d2, 1)) <= 0;
}

float
glyphy_arc_wedge_signed_dist (const glyphy_arc_t a, const vec2 p)
{
  vec2 c = glyphy_arc_center (a);
  return sign (a.d) * (distance (p, c) - distance (a.p0, c));
}

float
glyphy_arc_extended_dist (const glyphy_arc_t a, const vec2 p)
{
  vec2 m = mix (a.p0, a.p1, .5);
  float d2 = glyphy_tan2atan (a.d);
  if (dot (p - m, a.p1 - m) < 0)
    return dot (p - a.p0, normalize ((a.p1 - a.p0) * mat2(+d2, -1, +1, +d2)));
  else
    return dot (p - a.p1, normalize ((a.p1 - a.p0) * mat2(-d2, -1, +1, -d2)));
}

/* Returns grid width,height */
ivec2
glyphy_glyph_layout_decode (int glyph_layout)
{
  return ivec2 (mod (glyph_layout, 256), glyph_layout / 256);
}

int
glyphy_arc_list_offset (const vec2 p, int glyph_layout)
{
  ivec2 grid_size = glyphy_glyph_layout_decode (glyph_layout);
  ivec2 cell = ivec2 (clamp (p, vec2 (0,0), vec2(1,1) * (1.-GLYPHY_EPSILON)) * vec2 (grid_size));
  return cell.y * grid_size.x + cell.x;
}

glyphy_arc_list_t
glyphy_arc_list_decode (const vec4 v)
{
  ivec4 iv = glyphy_vec4_to_bytes (v);
  int offset = (iv.r * 256) + iv.g;
  int num_endpoints = iv.a;
  int side = 0; /* unsure */
  if (num_endpoints == 255) {
    num_endpoints = 0;
    side = -1;
  } else if (num_endpoints == 0)
    side = +1;
  return glyphy_arc_list_t (offset, num_endpoints, side);
}
