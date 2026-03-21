/*
 * Copyright 2022 Behdad Esfahbod. All Rights Reserved.
 *
 */

/* Intentionally doesn't have include guards */

#include "glyphy.h"

#ifdef __cplusplus
extern "C" {
#endif


#include <hb.h>



#ifndef GLYPHY_HARFBUZZ_PREFIX
#define GLYPHY_HARFBUZZ_PREFIX glyphy_harfbuzz_
#endif

#ifndef glyphy_harfbuzz
#define glyphy_harfbuzz(name) GLYPHY_PASTE (GLYPHY_HARFBUZZ_PREFIX, name)
#endif



static void
glyphy_harfbuzz(move_to) (hb_draw_funcs_t *dfuncs,
			  glyphy_curve_accumulator_t *acc,
			  hb_draw_state_t *st,
			  float to_x, float to_y,
			  void *user_data)
{
  glyphy_point_t p1 = {(double) to_x, (double) to_y};
  glyphy_curve_accumulator_close_path (acc);
  glyphy_curve_accumulator_move_to (acc, &p1);
}

static void
glyphy_harfbuzz(line_to) (hb_draw_funcs_t *dfuncs,
			  glyphy_curve_accumulator_t *acc,
			  hb_draw_state_t *st,
			  float to_x, float to_y,
			  void *user_data)
{
  glyphy_point_t p1 = {(double) to_x, (double) to_y};
  glyphy_curve_accumulator_line_to (acc, &p1);
}

static void
glyphy_harfbuzz(quadratic_to) (hb_draw_funcs_t *dfuncs,
			       glyphy_curve_accumulator_t *acc,
			       hb_draw_state_t *st,
			       float control_x, float control_y,
			       float to_x, float to_y,
			       void *user_data)
{
  glyphy_point_t p1 = {(double) control_x, (double) control_y};
  glyphy_point_t p2 = {(double) to_x, (double) to_y};
  glyphy_curve_accumulator_conic_to (acc, &p1, &p2);
}

static void
glyphy_harfbuzz(cubic_to) (hb_draw_funcs_t *dfuncs,
			   glyphy_curve_accumulator_t *acc,
			   hb_draw_state_t *st,
			   float control1_x, float control1_y,
			   float control2_x, float control2_y,
			   float to_x, float to_y,
			   void *user_data)
{
  /* TODO: cubics not supported yet; need cu2qu converter */
  (void) control1_x; (void) control1_y;
  (void) control2_x; (void) control2_y;
  glyphy_point_t p1 = {(double) to_x, (double) to_y};
  glyphy_curve_accumulator_line_to (acc, &p1);
}

static void
glyphy_harfbuzz(close_path) (hb_draw_funcs_t *dfuncs,
			     glyphy_curve_accumulator_t *acc,
			     hb_draw_state_t *st,
			     void *user_data)
{
  glyphy_curve_accumulator_close_path (acc);
}

static hb_draw_funcs_t *
glyphy_harfbuzz(get_draw_funcs) (void)
{
  static hb_draw_funcs_t *dfuncs = NULL;

  if (!dfuncs)
  {
    dfuncs = hb_draw_funcs_create ();
    hb_draw_funcs_set_move_to_func (dfuncs, (hb_draw_move_to_func_t) glyphy_harfbuzz(move_to), NULL, NULL);
    hb_draw_funcs_set_line_to_func (dfuncs, (hb_draw_line_to_func_t) glyphy_harfbuzz(line_to), NULL, NULL);
    hb_draw_funcs_set_quadratic_to_func (dfuncs, (hb_draw_quadratic_to_func_t) glyphy_harfbuzz(quadratic_to), NULL, NULL);
    hb_draw_funcs_set_cubic_to_func (dfuncs, (hb_draw_cubic_to_func_t) glyphy_harfbuzz(cubic_to), NULL, NULL);
    hb_draw_funcs_set_close_path_func (dfuncs, (hb_draw_close_path_func_t) glyphy_harfbuzz(close_path), NULL, NULL);
    hb_draw_funcs_make_immutable (dfuncs);
  }

  return dfuncs;
}

static void
glyphy_harfbuzz(font_get_glyph_shape) (hb_font_t *font,
				       hb_codepoint_t glyph,
				       glyphy_curve_accumulator_t *acc)
{
  hb_font_draw_glyph (font, glyph, glyphy_harfbuzz(get_draw_funcs) (), acc);
}

#ifdef __cplusplus
}
#endif
