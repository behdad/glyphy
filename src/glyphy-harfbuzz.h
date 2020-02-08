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

/* Intentionally doesn't have include guards */

#include "glyphy.h"

#ifdef __cplusplus
extern "C" {
#endif


#include "../../harfbuzz/src/hb.h"



#ifndef GLYPHY_HARFBUZZ_PREFIX
#define GLYPHY_HARFBUZZ_PREFIX glyphy_harfbuzz_
#endif

#ifndef glyphy_harfbuzz
#define glyphy_harfbuzz(name) GLYPHY_PASTE (GLYPHY_HARFBUZZ_PREFIX, name)
#endif



static void
glyphy_harfbuzz(move_to) (hb_position_t to_x, hb_position_t to_y,
			  glyphy_arc_accumulator_t *acc)
{
  glyphy_point_t p1 = {(double) to_x, (double) to_y};
  glyphy_arc_accumulator_close_path (acc);
  glyphy_arc_accumulator_move_to (acc, &p1);
  glyphy_arc_accumulator_successful (acc);
//   return glyphy_arc_accumulator_successful (acc) ? FT_Err_Ok : FT_Err_Out_Of_Memory;
}

static void
glyphy_harfbuzz(line_to) (hb_position_t to_x, hb_position_t to_y,
			  glyphy_arc_accumulator_t *acc)
{
  glyphy_point_t p1 = {(double) to_x, (double) to_y};
  glyphy_arc_accumulator_line_to (acc, &p1);
  glyphy_arc_accumulator_successful (acc);
//   return glyphy_arc_accumulator_successful (acc) ? FT_Err_Ok : FT_Err_Out_Of_Memory;
}

static void
glyphy_harfbuzz(quadratic_to) (hb_position_t control_x, hb_position_t control_y,
			       hb_position_t to_x, hb_position_t to_y,
			       glyphy_arc_accumulator_t *acc)
{
  glyphy_point_t p1 = {(double) control_x, (double) control_y};
  glyphy_point_t p2 = {(double) to_x, (double) to_y};
  glyphy_arc_accumulator_quadratic_to (acc, &p1, &p2);
  glyphy_arc_accumulator_successful (acc);
//   return glyphy_arc_accumulator_successful (acc) ? FT_Err_Ok : FT_Err_Out_Of_Memory;
}

static void
glyphy_harfbuzz(cubic_to) (hb_position_t control1_x, hb_position_t control1_y,
			   hb_position_t control2_x, hb_position_t control2_y,
			   hb_position_t to_x, hb_position_t to_y,
			   glyphy_arc_accumulator_t *acc)
{
  glyphy_point_t p1 = {(double) control1_x, (double) control1_y};
  glyphy_point_t p2 = {(double) control2_x, (double) control2_y};
  glyphy_point_t p3 = {(double) to_x, (double) to_y};
  glyphy_arc_accumulator_cubic_to (acc, &p1, &p2, &p3);
  glyphy_arc_accumulator_successful (acc);
//   return glyphy_arc_accumulator_successful (acc) ? FT_Err_Ok : FT_Err_Out_Of_Memory;
}

hb_draw_funcs_t *funcs = NULL;

static hb_bool_t
glyphy_harfbuzz(glyph_draw) (hb_font_t *font, hb_codepoint_t glyph,
			     glyphy_arc_accumulator_t *acc)
{
  if (!funcs)
  {
    funcs = hb_draw_funcs_create ();
    hb_draw_funcs_set_move_to_func (funcs, (hb_draw_move_to_func_t) glyphy_harfbuzz (move_to));
    hb_draw_funcs_set_line_to_func (funcs, (hb_draw_line_to_func_t) glyphy_harfbuzz (line_to));
    hb_draw_funcs_set_quadratic_to_func (funcs, (hb_draw_quadratic_to_func_t) glyphy_harfbuzz (quadratic_to));
    hb_draw_funcs_set_cubic_to_func (funcs, (hb_draw_cubic_to_func_t) glyphy_harfbuzz (cubic_to));
  }

  return hb_font_draw_glyph (font, glyph, funcs, acc);
}

#ifdef __cplusplus
}
#endif
