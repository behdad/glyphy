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
 * Google Author(s): Behdad Esfahbod, Maysum Panju, Wojciech Baranowski
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glyphy.h>

#include "glyphy-geometry.hh"
#include "glyphy-arcs-bezier.hh"

using namespace GLyphy::Geometry;
using namespace GLyphy::ArcsBezier;



/*
 * Approximate outlines with multiple arcs
 */


void
glyphy_arc_accumulator_init (glyphy_arc_accumulator_t *acc,
			     double                    tolerance,
			     glyphy_arc_endpoint_accumulator_callback_t callback,
			     void                     *user_data)
{
  acc->current_point = Point (0, 0);
  acc->num_endpoints = 0;
  acc->tolerance = tolerance;
  acc->max_error = 0;
  acc->success = true;
  acc->callback = callback;
  acc->user_data = user_data;
}

static void
accumulate (glyphy_arc_accumulator_t *acc, const Point &p, double d)
{
  glyphy_arc_endpoint_t endpoint = {p.x, p.y, d};
  acc->success = acc->success && acc->callback (acc, &endpoint, acc->user_data);
  if (acc->success) {
    acc->num_endpoints++;
    acc->current_point = p;
  }
}

static void
move_to (glyphy_arc_accumulator_t *acc, const Point &p)
{
  if (!acc->num_endpoints || p != acc->current_point)
    accumulate (acc, p, INFINITY);
}

static void
arc (glyphy_arc_accumulator_t *acc, const Arc &a)
{
  move_to (acc, a.p0);
  accumulate (acc, a.p1, a.d);
}

static void
bezier (glyphy_arc_accumulator_t *acc, const Bezier &b)
{
  double e;
  std::vector<Arc> arcs;
  ArcsBezierApproximatorDefault::approximate_bezier_with_arcs (b, acc->tolerance, arcs, &e);
  acc->max_error = std::max (acc->max_error, e);

  for (unsigned int i = 0; i < arcs.size (); i++)
    arc (acc, arcs[i]);
}

void
glyphy_arc_accumulator_move_to (glyphy_arc_accumulator_t *acc,
				glyphy_point_t p0)
{
  move_to (acc, p0);
}

void
glyphy_arc_accumulator_line_to (glyphy_arc_accumulator_t *acc,
				glyphy_point_t p1)
{
  arc (acc, Arc (acc->current_point, p1, 0));
}

void
glyphy_arc_accumulator_conic_to (glyphy_arc_accumulator_t *acc,
				 glyphy_point_t p1,
				 glyphy_point_t p2)
{
  glyphy_arc_accumulator_cubic_to (acc,
				   Point (acc->current_point).lerp (2/3., p1),
				   Point (p2).lerp (2/3., p1),
				   p2);
}

void
glyphy_arc_accumulator_cubic_to (glyphy_arc_accumulator_t *acc,
				 glyphy_point_t p1,
				 glyphy_point_t p2,
				 glyphy_point_t p3)
{
  bezier (acc, Bezier (acc->current_point, p1, p2, p3));
}

void
glyphy_arc_accumulator_arc_to (glyphy_arc_accumulator_t *acc,
			       glyphy_point_t p1,
			       double         d)
{
  arc (acc, Arc (acc->current_point, p1, d));
}
