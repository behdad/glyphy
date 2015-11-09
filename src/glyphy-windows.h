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

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef GLYPHY_WINDOWS__PREFIX
#define GLYPHY_WINDOWS_PREFIX glyphy_windows_
#endif

#ifndef glyphy_windows
#define glyphy_windows(name) GLYPHY_PASTE (GLYPHY_WINDOWS_PREFIX, name)
#endif

static double fixed_to_double(FIXED f)
{
  return f.value + f.fract / double(0x10000);
}

static int
glyphy_windows(move_to) (const POINTFX            *to,
			 glyphy_arc_accumulator_t *acc)
{
  glyphy_point_t p1 = {fixed_to_double(to->x), fixed_to_double(to->y)};
  glyphy_arc_accumulator_close_path (acc);
  glyphy_arc_accumulator_move_to (acc, &p1);
  return glyphy_arc_accumulator_successful (acc) ? 0 : -1;
}

static int
glyphy_windows(line_to) (const POINTFX            *to,
			 glyphy_arc_accumulator_t *acc)
{
  glyphy_point_t p1 = {fixed_to_double(to->x), fixed_to_double(to->y)};
  glyphy_arc_accumulator_line_to (acc, &p1);
  return glyphy_arc_accumulator_successful (acc) ? 0 : -1;
}

static int
glyphy_windows(conic_to) (const POINTFX            *control,
			  const glyphy_point_t     *p2,
			  glyphy_arc_accumulator_t *acc)
{
  glyphy_point_t p1 = {fixed_to_double(control->x), fixed_to_double(control->y)};
  glyphy_arc_accumulator_conic_to (acc, &p1, p2);
  return glyphy_arc_accumulator_successful (acc) ? 0 : -1;
}

/* See https://support.microsoft.com/en-us/kb/87115 */

static int
glyphy_windows(outline_decompose) (const TTPOLYGONHEADER    *outline,
				   size_t                    outline_size,
				   glyphy_arc_accumulator_t *acc)
{
  const TTPOLYGONHEADER *polygon = outline;
  const TTPOLYGONHEADER *outline_end = (const TTPOLYGONHEADER*) ((char *)outline + outline_size);

  int polygon_count = 0;
  while (polygon < outline_end)
  {
    if (((char *)polygon + polygon->cb) > (char *) outline_end)
      die ("TTPOLYGONHEADER record too large for enclosing data");

    assert(polygon->dwType == TT_POLYGON_TYPE);

    if (glyphy_windows(move_to) (&polygon->pfxStart, acc) == -1)
      return -1;

    const TTPOLYCURVE *curve = (const TTPOLYCURVE*) (polygon + 1);
    const TTPOLYCURVE *curve_end = (const TTPOLYCURVE*) ((char *)polygon + polygon->cb);
    int curve_count = 0;
    while (curve < curve_end)
    {
      if (((char *) &curve->apfx[curve->cpfx]) > (char *) curve_end)
	die ("TTPOLYCURVE record too large for enclosing TTPOLYGONHEADER\n");

      switch (curve->wType)
      {
      case TT_PRIM_LINE:
	{
	  int i;
	  for (i = 0; i < curve->cpfx; i++) {
	    if (glyphy_windows(line_to) (&curve->apfx[i], acc) == -1)
	      return -1;
	  }

	  /* A final TT_PRIM_LINE in a contour automatically closes to the contour start */
	  if ((const TTPOLYCURVE *) ((char *) &curve->apfx[curve->cpfx]) == curve_end)
	    if (glyphy_windows(line_to) (&polygon->pfxStart, acc) == -1)
	      return -1;
	}
	break;
      case TT_PRIM_QSPLINE:
	{
	  int i = 0;
	  while (i < curve->cpfx) {
	    const POINTFX *control = &curve->apfx[i];
	    i++;
	    const POINTFX *p2 = &curve->apfx[i];
	    glyphy_point_t p2pt = {fixed_to_double(p2->x), fixed_to_double(p2->y)};
	    if (i == curve->cpfx - 1) {
	      i++;
	    } else {
	      p2pt.x = (p2pt.x + fixed_to_double(control->x)) / 2;
	      p2pt.y = (p2pt.y + fixed_to_double(control->y)) / 2;
	    }
	    if (glyphy_windows(conic_to) (control, &p2pt, acc) == -1)
	      return -1;
	  }
	  /* If the last point of the contour was not the start point, draw a closing line to it */
	  if ((const TTPOLYCURVE *) ((char *) &curve->apfx[curve->cpfx]) == curve_end)
	    if (glyphy_windows(line_to) (&polygon->pfxStart, acc) == -1)
	      return -1;
	}
	break;
      default:
	die ("Unexpected record in TTPOLYCURVE");
      }
      curve = (const TTPOLYCURVE *) ((char *) &curve->apfx[curve->cpfx]);
    }
    polygon = (const TTPOLYGONHEADER *) curve_end;
  }
  return 0;
}

#ifdef __cplusplus
}
#endif
