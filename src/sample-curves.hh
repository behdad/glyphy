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

#include <glyphy/geometry.hh>

#ifndef SAMPLE_CURVES_HH
#define SAMPLE_CURVES_HH

namespace GLyphy {

namespace SampleCurves {

using namespace Geometry;

#define B Bezier<Coord>
#define P Point<Coord>

static inline const Bezier<Coord>
sample_curve_riskus_simple (void)
{
  return B (P (16.9753, .7421),
	    P (18.2203, 2.2238),
	    P (21.0939, 2.4017),
	    P (23.1643, 1.6148));
}

static inline const Bezier<Coord>
sample_curve_riskus_complicated (void)
{
  return B (P (17.5415, 0.9003),
	    P (18.4778, 3.8448),
	    P (22.4037, -0.9109),
	    P (22.563, 0.7782));
}

static inline const Bezier<Coord>
sample_curve_s (void)
{
  return B (P (18.4778, 3.8448),
	    P (17.5415, 0.9003),
	    P (22.563, 0.7782),
	    P (22.4037, -0.9109));
}

static inline const Bezier<Coord>
sample_curve_skewed (void)
{
  return  B (P (2.5, 19.0),
	     P (0, -10.0),
	     P (25.0, -5.0),
	     P (33.0, 1.0));
}


static inline const Bezier<Coord>
sample_curve_serpentine_c_symmetric (void)
{
  return B (P (0, 0),
	    P (0, 10),
	    P (10, 10),
	    P (10, 0));
}

static inline const Bezier<Coord>
sample_curve_serpentine_s_symmetric (void)
{
  return B (P (0, 0),
	    P (0, 1),
	    P (1, -1),
	    P (1, 0));
}

static inline const Bezier<Coord>
sample_curve_serpentine_quadratic (void)
{
  return B (P (0, 0),
	    P (2, 6),
	    P (4, 6),
	    P (6, 0));
}

static inline const Bezier<Coord>
sample_curve_loop_cusp_symmetric (void)
{
  return B (P (0, 0),
	    P (10, 10),
	    P (0, 10),
	    P (10, 0));
}

static inline const Bezier<Coord>
sample_curve_loop_gamma_symmetric (void)
{
  return B (P (0, 0),
	    P (21, 21),
	    P (-14, 21),
	    P (7, 0));
}

static inline const Bezier<Coord>
sample_curve_loop_gamma_small_symmetric (void)
{
  return B (P (0, 0),
	    P (11, 11),
	    P (-1, 11),
	    P (10, 0));
}

static inline const Bezier<Coord>
sample_curve_loop_o_symmetric (void)
{
  return B (P (0, 0),
	    P (3, 3),
	    P (-3, 3),
	    P (0, 0));
}

static inline const Bezier<Coord>
sample_curve_semicircle_top (void)
{
  return B (P (10, 15),
	    P (10, 11),
	    P (16, 11),
	    P (16, 15));
}

static inline const Bezier<Coord>
sample_curve_semicircle_bottom (void)
{
  return B (P (10, 10),
	    P (10, 14),
	    P (16, 14),
	    P (16, 10));
}

static inline const Bezier<Coord>
sample_curve_semicircle_left (void)
{
  return B (P (16, 16),
	    P (12, 16),
	    P (12, 10),
	    P (16, 10));
}

static inline const Bezier<Coord>
sample_curve_semicircle_right (void)
{
  return B (P (10, 16),
	    P (14, 16),
	    P (14, 10),
	    P (10, 10));
}


#undef B
#undef P

} /* namespace SampleCurves */

} /* namespace GLyphy */
#endif
