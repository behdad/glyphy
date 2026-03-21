/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy.h"

#include <cstdlib>


/*
 * Accumulate quadratic Bezier curves from glyph outlines
 */


struct glyphy_curve_accumulator_t {
  unsigned int refcount;

  glyphy_curve_accumulator_callback_t callback;
  void                               *user_data;

  glyphy_point_t start_point;
  glyphy_point_t current_point;
  bool           need_moveto;
  unsigned int   num_curves;
  glyphy_bool_t  success;
};


glyphy_curve_accumulator_t *
glyphy_curve_accumulator_create (void)
{
  glyphy_curve_accumulator_t *acc = (glyphy_curve_accumulator_t *) calloc (1, sizeof (glyphy_curve_accumulator_t));
  acc->refcount = 1;

  acc->callback = NULL;
  acc->user_data = NULL;

  glyphy_curve_accumulator_reset (acc);

  return acc;
}

void
glyphy_curve_accumulator_reset (glyphy_curve_accumulator_t *acc)
{
  acc->start_point = acc->current_point = {0, 0};
  acc->need_moveto = true;
  acc->num_curves = 0;
  acc->success = true;
}

void
glyphy_curve_accumulator_destroy (glyphy_curve_accumulator_t *acc)
{
  if (!acc || --acc->refcount)
    return;

  free (acc);
}

glyphy_curve_accumulator_t *
glyphy_curve_accumulator_reference (glyphy_curve_accumulator_t *acc)
{
  if (acc)
    acc->refcount++;
  return acc;
}


/* Configure acc */

void
glyphy_curve_accumulator_set_callback (glyphy_curve_accumulator_t *acc,
				       glyphy_curve_accumulator_callback_t callback,
				       void                       *user_data)
{
  acc->callback = callback;
  acc->user_data = user_data;
}

void
glyphy_curve_accumulator_get_callback (glyphy_curve_accumulator_t  *acc,
				       glyphy_curve_accumulator_callback_t *callback,
				       void                       **user_data)
{
  *callback = acc->callback;
  *user_data = acc->user_data;
}


/* Accumulation results */

unsigned int
glyphy_curve_accumulator_get_num_curves (glyphy_curve_accumulator_t *acc)
{
  return acc->num_curves;
}

glyphy_bool_t
glyphy_curve_accumulator_successful (glyphy_curve_accumulator_t *acc)
{
  return acc->success;
}


/* Accumulate */

static void
emit (glyphy_curve_accumulator_t *acc, const glyphy_curve_t *curve)
{
  acc->success = acc->success && acc->callback (curve, acc->user_data);
  if (acc->success) {
    acc->num_curves++;
    acc->current_point = curve->p3;
  }
}

static void
emit_conic (glyphy_curve_accumulator_t *acc,
	    const glyphy_point_t *p2,
	    const glyphy_point_t *p3)
{
  if (acc->current_point.x == p3->x && acc->current_point.y == p3->y)
    return;

  if (acc->need_moveto) {
    acc->start_point = acc->current_point;
    acc->need_moveto = false;
  }

  glyphy_curve_t curve = {acc->current_point, *p2, *p3};
  emit (acc, &curve);
}


void
glyphy_curve_accumulator_move_to (glyphy_curve_accumulator_t *acc,
				  const glyphy_point_t *p0)
{
  acc->need_moveto = true;
  acc->current_point = *p0;
}

void
glyphy_curve_accumulator_line_to (glyphy_curve_accumulator_t *acc,
				  const glyphy_point_t *p1)
{
  /* Line as degenerate quadratic: p2 = p1 (start point).
   * This gives a = p3 - p1 (never near zero for non-horizontal lines),
   * b = 0, avoiding float32 precision issues in the GPU solver. */
  glyphy_point_t p0 = acc->current_point;
  emit_conic (acc, &p0, p1);
}

void
glyphy_curve_accumulator_conic_to (glyphy_curve_accumulator_t *acc,
				   const glyphy_point_t *p1,
				   const glyphy_point_t *p2)
{
  emit_conic (acc, p1, p2);
}

void
glyphy_curve_accumulator_close_path (glyphy_curve_accumulator_t *acc)
{
  if (!acc->need_moveto &&
      (acc->current_point.x != acc->start_point.x ||
       acc->current_point.y != acc->start_point.y))
    glyphy_curve_accumulator_line_to (acc, &acc->start_point);
}


/*
 * Curve list extents
 */

void
glyphy_curve_list_extents (const glyphy_curve_t *curves,
			   unsigned int          num_curves,
			   glyphy_extents_t     *extents)
{
  glyphy_extents_clear (extents);
  for (unsigned int i = 0; i < num_curves; i++) {
    const glyphy_curve_t &c = curves[i];
    /* Conservative: use control point bounding box */
    glyphy_extents_add (extents, &c.p1);
    glyphy_extents_add (extents, &c.p2);
    glyphy_extents_add (extents, &c.p3);
  }
}
