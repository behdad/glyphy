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

#ifndef GLYPHY_FREETYPE_H
#define GLYPHY_FREETYPE_H

#include "glyphy.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H



#ifndef GLYPHY_FREETYPE_PREFIX
#define GLYPHY_FREETYPE_PREFIX glyphy_freetype_
#endif

#define glyphy_freetype(name) GLYPHY_PASTE (GLYPHY_FREETYPE_PREFIX, name)



static FT_Error glyphy_freetype(err) (glyphy_arc_accumulator_t *acc)
{
  return acc->success ? FT_Err_Ok : FT_Err_Out_Of_Memory;
}

static const glyphy_point_t glyphy_freetype(point) (FT_Vector *to)
{
  glyphy_point_t p = {to->x, to->y};
  return p;
}

static int
glyphy_freetype(move_to) (FT_Vector *to,
			  glyphy_arc_accumulator_t *acc)
{
  glyphy_arc_accumulator_move_to (acc,
				  glyphy_freetype(point) (to));
  return glyphy_freetype(err) (acc);
}

static int
glyphy_freetype(line_to) (FT_Vector *to,
			  glyphy_arc_accumulator_t *acc)
{
  glyphy_arc_accumulator_line_to (acc,
				  glyphy_freetype(point) (to));
  return glyphy_freetype(err) (acc);
}

static int
glyphy_freetype(conic_to) (FT_Vector *control, FT_Vector *to,
			   glyphy_arc_accumulator_t *acc)
{
  glyphy_arc_accumulator_conic_to (acc,
				   glyphy_freetype(point) (control),
				   glyphy_freetype(point) (to));
  return glyphy_freetype(err) (acc);
}

static int
glyphy_freetype(cubic_to) (FT_Vector *control1, FT_Vector *control2, FT_Vector *to,
			   glyphy_arc_accumulator_t *acc)
{
  glyphy_arc_accumulator_cubic_to (acc,
				   glyphy_freetype(point) (control1),
				   glyphy_freetype(point) (control2),
				   glyphy_freetype(point) (to));
  return glyphy_freetype(err) (acc);
}

static FT_Error
glyphy_freetype(outline_decompose) (const FT_Outline *outline,
				    glyphy_arc_accumulator_t acc)
{
  static const FT_Outline_Funcs outline_funcs = {
    (FT_Outline_MoveToFunc) glyphy_freetype(move_to),
    (FT_Outline_LineToFunc) glyphy_freetype(line_to),
    (FT_Outline_ConicToFunc) glyphy_freetype(conic_to),
    (FT_Outline_CubicToFunc) glyphy_freetype(cubic_to),
    0, /* shift */
    0, /* delta */
  };

  return FT_Outline_Decompose (const_cast <FT_Outline *> (outline), &outline_funcs, &acc);
}



#endif /* GLYPHY_FREETYPE_H */
