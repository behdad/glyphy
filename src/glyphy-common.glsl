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

vec2
/*<*/glyphy_perpendicular/*>*/ (const vec2 v)
{
  return vec2 (-v.y, v.x);
}

int
/*<*/glyphy_float_to_byte/*>*/ (const float v)
{
  return int (v * (256 - 1e-5));
}

/* returns tan (2 * atan (d)) */
float
/*<*/glyphy_tan2atan/*>*/ (float d)
{
  return 2 * d / (1 - d * d);
}

vec3
/*<*/glyphy_arc_decode/*>*/ (const vec4 v)
{
  float d = v.r;
  float x = (float (mod (/*<*/glyphy_float_to_byte/*>*/ (v.a) / 16, 16)) + v.g) / 16;
  float y = (float (mod (/*<*/glyphy_float_to_byte/*>*/ (v.a)     , 16)) + v.b) / 16;
  d = .5 /*MAX_D*/ * (2 * d - 1);
  return vec3 (x, y, d);
}

vec2
/*<*/glyphy_arc_center/*>*/ (const vec2 p0, const vec2 p1, float d)
{
  return mix (p0, p1, .5) +
	 /*<*/glyphy_perpendicular/*>*/ (p1 - p0) / (2 * /*<*/glyphy_tan2atan/*>*/ (d));
}

float
/*<*/glyphy_arc_extended_dist/*>*/ (const vec2 p, const vec2 p0, const vec2 p1, float d)
{
  vec2 m = mix (p0, p1, .5);
  float d2 = /*<*/glyphy_tan2atan/*>*/ (d);
  if (dot (p - m, p1 - m) < 0)
    return dot (p - p0, normalize ((p1 - p0) * +mat2(d2, -1, +1, d2)));
  else
    return dot (p - p1, normalize ((p1 - p0) * -mat2(d2, +1, -1, d2)));
}

ivec3
/*<*/glyphy_arclist_decode/*>*/ (const vec4 v)
{
  int offset = (/*<*/glyphy_float_to_byte/*>*/ (v.r) * 256 +
		/*<*/glyphy_float_to_byte/*>*/ (v.g)) * 256 +
		/*<*/glyphy_float_to_byte/*>*/ (v.b);
  int num_points = /*<*/glyphy_float_to_byte/*>*/ (v.a);
  int side = 0; /* unsure */
  if (num_points == 255) {
    num_points = 0;
    side = -1;
  } else if (num_points == 0)
    side = +1;
  return ivec3 (offset, num_points, side);
}
