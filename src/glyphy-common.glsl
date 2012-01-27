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

#define GLYPHY_PASTE_ARGS(prefix, name) prefix ## name
#define GLYPHY_PASTE(prefix, name) GLYPHY_PASTE_ARGS (prefix, name)

#ifndef GLYPHY_PREFIX
#define GLYPHY_PREFIX glyphy_
#endif

#ifndef glyphy
#define glyphy(name) GLYPHY_PASTE (GLYPHY_PREFIX, name)
#endif


#ifndef GLYPHY_INFINITY
#define GLYPHY_INFINITY 1e9
#endif
#ifndef GLYPHY_EPSILON
#define GLYPHY_EPSILON  1e-5
#endif


struct glyphy(arc_t) {
  vec2  p0;
  vec2  p1;
  float d;
};

struct glyphy(arc_endpoint_t) {
  vec2  p;
  float d;
};

/* Return value is:
 * z: If num_endpoints is zero, this specifies whether we are inside (-1)
 *    or outside (+1).  Otherwise we're unsure (0).
 */
struct glyphy(arc_list_t) {
  /* Offset to the arc-endpoints from the beginning of the glyph blob */
  int offset;
  /* Number of endpoints in the list (may be zero) */
  int num_endpoints;
  /* If num_endpoints is zero, this specifies whether we are inside (-1)
   * or outside (+1).  Otherwise we're unsure (0). */
  int side;
};

bool
glyphy(isinf) (float v)
{
  return abs (v) > GLYPHY_INFINITY / 2.;
}

bool
glyphy(iszero) (float v)
{
  return abs (v) < GLYPHY_EPSILON * 2.;
}

vec2
glyphy(perpendicular) (const vec2 v)
{
  return vec2 (-v.y, v.x);
}

int
glyphy(float_to_byte) (const float v)
{
  return int (v * (256 - GLYPHY_EPSILON));
}

ivec4
glyphy(vec4_to_bytes) (const vec4 v)
{
  return ivec4 (v * (256 - GLYPHY_EPSILON));
}

ivec2
glyphy(float_to_two_nimbles) (const float v)
{
  int f = glyphy(float_to_byte) (v);
  return ivec2 (f / 16, mod (f, 16));
}

/* returns tan (2 * atan (d)) */
float
glyphy(tan2atan) (float d)
{
  return 2 * d / (1 - d * d);
}

glyphy(arc_endpoint_t)
glyphy(arc_endpoint_decode) (const vec4 v)
{
  /* Note that this never returns d == 0.  For straight lines,
   * a d value of .0039215686 is returned.  In fact, the d has
   * that bias for all values.
   */
  vec2 p = (vec2 (glyphy(float_to_two_nimbles) (v.a)) + v.gb) / 16;
  float d = v.r;
  if (d == 0)
    d = GLYPHY_INFINITY;
#define GLYPHY_MAX_D .5
    d = GLYPHY_MAX_D * (2 * d - 1);
#undef GLYPHY_MAX_D
  return glyphy(arc_endpoint_t) (p, d);
}

vec2
glyphy(arc_center) (glyphy(arc_t) a)
{
  return mix (a.p0, a.p1, .5) +
	 glyphy(perpendicular) (a.p1 - a.p0) / (2 * glyphy(tan2atan) (a.d));
}

float
glyphy(arc_extended_dist) (const vec2 p, glyphy(arc_t) a)
{
  vec2 m = mix (a.p0, a.p1, .5);
  float d2 = glyphy(tan2atan) (a.d);
  if (dot (p - m, a.p1 - m) < 0)
    return dot (p - a.p0, normalize ((a.p1 - a.p0) * mat2(+d2, -1, +1, +d2)));
  else
    return dot (p - a.p1, normalize ((a.p1 - a.p0) * mat2(-d2, -1, +1, -d2)));
}

glyphy(arc_list_t)
glyphy(arc_list_decode) (const vec4 v)
{
  ivec4 iv = glyphy(vec4_to_bytes) (v) * ivec4 (65536, 256, 1, 1);
  int offset = iv.r + iv.g + iv.b;
  int num_endpoints = iv.a;
  int side = 0; /* unsure */
  if (num_endpoints == 255) {
    num_endpoints = 0;
    side = -1;
  } else if (num_endpoints == 0)
    side = +1;
  return glyphy(arc_list_t) (offset, num_endpoints, side);
}
