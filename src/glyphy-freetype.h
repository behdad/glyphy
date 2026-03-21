/*

 * Copyright 2012 Google, Inc. All Rights Reserved.
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 */

/* Intentionally doesn't have include guards */

#include "glyphy.h"

#ifdef __cplusplus
extern "C" {
#endif


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H



#ifndef GLYPHY_FREETYPE_PREFIX
#define GLYPHY_FREETYPE_PREFIX glyphy_freetype_
#endif

#ifndef glyphy_freetype
#define glyphy_freetype(name) GLYPHY_PASTE (GLYPHY_FREETYPE_PREFIX, name)
#endif



static int
glyphy_freetype(move_to) (FT_Vector *to,
			  glyphy_curve_accumulator_t *acc)
{
  glyphy_point_t p1 = {(double) to->x, (double) to->y};
  glyphy_curve_accumulator_close_path (acc);
  glyphy_curve_accumulator_move_to (acc, &p1);
  return glyphy_curve_accumulator_successful (acc) ? FT_Err_Ok : FT_Err_Out_Of_Memory;
}

static int
glyphy_freetype(line_to) (FT_Vector *to,
			  glyphy_curve_accumulator_t *acc)
{
  glyphy_point_t p1 = {(double) to->x, (double) to->y};
  glyphy_curve_accumulator_line_to (acc, &p1);
  return glyphy_curve_accumulator_successful (acc) ? FT_Err_Ok : FT_Err_Out_Of_Memory;
}

static int
glyphy_freetype(conic_to) (FT_Vector *control, FT_Vector *to,
			   glyphy_curve_accumulator_t *acc)
{
  glyphy_point_t p1 = {(double) control->x, (double) control->y};
  glyphy_point_t p2 = {(double) to->x, (double) to->y};
  glyphy_curve_accumulator_conic_to (acc, &p1, &p2);
  return glyphy_curve_accumulator_successful (acc) ? FT_Err_Ok : FT_Err_Out_Of_Memory;
}

static int
glyphy_freetype(cubic_to) (FT_Vector *control1, FT_Vector *control2, FT_Vector *to,
			   glyphy_curve_accumulator_t *acc)
{
  /* TODO: cubics not supported yet; need cu2qu converter */
  (void) control1; (void) control2;
  glyphy_point_t p1 = {(double) to->x, (double) to->y};
  glyphy_curve_accumulator_line_to (acc, &p1);
  return glyphy_curve_accumulator_successful (acc) ? FT_Err_Ok : FT_Err_Out_Of_Memory;
}

static FT_Error
glyphy_freetype(outline_decompose) (const FT_Outline              *outline,
				    glyphy_curve_accumulator_t    *acc)
{
  const FT_Outline_Funcs outline_funcs = {
    (FT_Outline_MoveToFunc) glyphy_freetype(move_to),
    (FT_Outline_LineToFunc) glyphy_freetype(line_to),
    (FT_Outline_ConicToFunc) glyphy_freetype(conic_to),
    (FT_Outline_CubicToFunc) glyphy_freetype(cubic_to),
    0, /* shift */
    0, /* delta */
  };

  return FT_Outline_Decompose ((FT_Outline *) outline, &outline_funcs, acc);
}

#ifdef __cplusplus
}
#endif
