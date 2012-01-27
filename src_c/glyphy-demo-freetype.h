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

#ifndef GLYPHY_DEMO_FREETYPE_H
#define GLYPHY_DEMO_FREETYPE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <vector>



static FT_Error ft_err (glyphy_arc_accumulator_t *acc)
{
  return acc->success ? FT_Err_Ok : FT_Err_Out_Of_Memory;
}

static const glyphy_point_t ft_point (FT_Vector *to)
{
  glyphy_point_t p = {to->x, to->y};
  return p;
}

static int
ft_move_to (FT_Vector *to,
	    glyphy_arc_accumulator_t *acc)
{
  glyphy_arc_accumulator_move_to (acc, ft_point (to));
  return ft_err (acc);
}

static int
ft_line_to (FT_Vector *to,
	    glyphy_arc_accumulator_t *acc)
{
  glyphy_arc_accumulator_line_to (acc, ft_point (to));
  return ft_err (acc);
}

static int
ft_conic_to (FT_Vector *control, FT_Vector *to,
	     glyphy_arc_accumulator_t *acc)
{
  glyphy_arc_accumulator_conic_to (acc, ft_point (control), ft_point (to));
  return ft_err (acc);
}

static int
ft_cubic_to (FT_Vector *control1, FT_Vector *control2, FT_Vector *to,
	     glyphy_arc_accumulator_t *acc)
{
  glyphy_arc_accumulator_cubic_to (acc, ft_point (control1), ft_point (control2), ft_point (to));
  return ft_err (acc);
}

static glyphy_bool_t
ft_accumulate_endpoint (glyphy_arc_endpoint_t              *endpoint,
		        std::vector<glyphy_arc_endpoint_t> *endpoints)
{
  endpoints->push_back (*endpoint);
  return true;
}

static glyphy_bool_t
ft_outline_to_arcs (const FT_Outline *outline,
		    double tolerance,
		    std::vector<glyphy_arc_endpoint_t> &endpoints,
		    double *error)
{
  glyphy_arc_accumulator_t acc;
  glyphy_arc_accumulator_init (&acc, tolerance,
			       (glyphy_arc_endpoint_accumulator_callback_t) ft_accumulate_endpoint,
			       &endpoints);

  static const FT_Outline_Funcs outline_funcs = {
    (FT_Outline_MoveToFunc) ft_move_to,
    (FT_Outline_LineToFunc) ft_line_to,
    (FT_Outline_ConicToFunc) ft_conic_to,
    (FT_Outline_CubicToFunc) ft_cubic_to,
    0, /* shift */
    0, /* delta */
  };

  bool success = FT_Err_Ok == FT_Outline_Decompose (const_cast <FT_Outline *> (outline), &outline_funcs, &acc);
  if (error)
    *error = acc.max_error;

  return success;
}



#endif /* GLYPHY_DEMO_FREETYPE_H */
