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

#ifndef GLYPHY_RGBA_HH
#define GLYPHY_RGBA_HH

#include <glyphy-geometry.hh>

#include <assert.h>

#include <algorithm>

namespace GLyphy {
namespace RGBA {


#define UPPER_BITS(v,bits,total_bits) ((v) >> ((total_bits) - (bits)))
#define LOWER_BITS(v,bits,total_bits) ((v) & ((1 << (bits)) - 1))
#define MIDDLE_BITS(v,bits,upper_bound,total_bits) (UPPER_BITS (LOWER_BITS (v, upper_bound, total_bits), bits, upper_bound))


static inline glyphy_rgba_t
arc_encode (double x, double y, double d)
{
  glyphy_rgba_t v;

  /* 12 bits for each of x and y, 8 bits for d */
  unsigned int ix, iy, id;
  ix = lround (x * 4095);
  assert (ix < 4096);
  iy = lround (y * 4095);
  assert (iy < 4096);
#define GLYPHY_MAX_D .5
  if (isinf (d))
    id = 0;
  else {
    assert (fabs (d) < GLYPHY_MAX_D);
    id = lround (d * 127. / GLYPHY_MAX_D + 128);
  }
  assert (id < 256);

  v.r = id;
  v.g = LOWER_BITS (ix, 8, 12);
  v.b = LOWER_BITS (iy, 8, 12);
  v.a = ((ix >> 8) << 4) | (iy >> 8);
  return v;
}

static inline glyphy_rgba_t
arclist_encode (unsigned int offset, unsigned int num_points, bool is_inside)
{
  glyphy_rgba_t v;
  v.r = UPPER_BITS (offset, 8, 24);
  v.g = MIDDLE_BITS (offset, 8, 16, 24);
  v.b = LOWER_BITS (offset, 8, 24);
  v.a = LOWER_BITS (num_points, 8, 8);
  if (is_inside && !num_points)
    v.a = 255;
  return v;
}


} /* namespace RGBA */
} /* namespace GLyphy */

#endif /* GLYPHY_RGBA_HH */
