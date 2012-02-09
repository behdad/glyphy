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
 * Google Author(s): Behdad Esfahbod
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy-common.hh"
#include "glyphy-geometry.hh"

using namespace GLyphy::Geometry;


void
glyphy_outline_reverse (glyphy_arc_endpoint_t *endpoints,
			unsigned int           num_endpoints)
{
  // Shift the d's first
  double d0 = endpoints[0].d;
  for (unsigned int i = 0; i < num_endpoints - 1; i++)
    endpoints[i].d = -endpoints[i + 1].d;
  endpoints[num_endpoints - 1].d = d0;

  // Reverse
  for (unsigned int i = 0, j = num_endpoints - 1; i < j; i++, j--) {
    glyphy_arc_endpoint_t t = endpoints[i];
    endpoints[i] = endpoints[j];
    endpoints[j] = t;
  }
}


static glyphy_bool_t
winding (const glyphy_arc_endpoint_t *endpoints,
	 unsigned int                 num_endpoints)
{
  /*
   * Algorithm:
   *
   * - Find the lowest-x part of the contour,
   * - If the point is an endpoint:
   *   o compare the angle of the incoming and outgoing edges of that point
   *     to find out whether it's CW or CCW,
   * - Otherwise, compare the y of the two endpoints of the arc with lowest-x point.
   */

  unsigned int corner = 0;
  for (unsigned int i = 1; i < num_endpoints; i++)
    if (endpoints[i].p.x < endpoints[corner].p.x ||
	(endpoints[i].p.x == endpoints[corner].p.x &&
	 endpoints[i].p.y < endpoints[corner].p.y))
      corner = i;

  double min_x = endpoints[corner].p.x;
  int winner = -1;
  Point p0 (0, 0);
  for (unsigned int i = 0; i < num_endpoints; i++) {
    const glyphy_arc_endpoint_t &endpoint = endpoints[i];
    if (endpoint.d == INFINITY || endpoint.d == 0 /* arcs only, not lines */) {
      p0 = endpoint.p;
      continue;
    }
    Arc arc (p0, endpoint.p, endpoint.d);
    p0 = endpoint.p;

    Point c = arc.center ();
    double r = arc.radius ();
    if (c.x - r < min_x && arc.wedge_contains_point (c - Vector (r, 0))) {
      min_x = c.x - r;
      winner = i;
    }
  }

  if (winner == -1)
  {
    // Corner is lowest-x.  Find the tangents of the two arcs connected to the
    // corner and compare the tangent angles to get contour direction.
    const glyphy_arc_endpoint_t ethis = endpoints[corner];
    const glyphy_arc_endpoint_t eprev = endpoints[(corner + 1) % num_endpoints];
    const glyphy_arc_endpoint_t enext = endpoints[corner ? corner - 1 : num_endpoints - 1];
    double in  = (-Arc (eprev.p, ethis.p, ethis.d).tangents ().second).angle (); 
    double out = (+Arc (ethis.p, enext.p, enext.d).tangents ().first ).angle (); 
    return in < out;
  }
  else
  {
    // Easy.
    return endpoints[winner].d > 0;
  }

  return false;
}

static glyphy_bool_t
even_odd (const glyphy_arc_endpoint_t *endpoints,
	  unsigned int                 num_endpoints,
	  const glyphy_arc_endpoint_t *all_endpoints,
	  unsigned int                 all_num_endpoints)
{
  /*
   * Algorithm:
   *
   * - For a point on the contour, draw a line in a direction (eg. decreasing x) to infinity,
   * - Count how many times it crosses all contours,
   *   Pay special attention to points falling exactly on the line and arcs crossing more than once.
   */

  return false;
}

static glyphy_bool_t
process_contour (glyphy_arc_endpoint_t       *endpoints,
		 unsigned int                 num_endpoints,
		 const glyphy_arc_endpoint_t *all_endpoints,
		 unsigned int                 all_num_endpoints,
		 glyphy_bool_t                inverse)
{
  /*
   * Algorithm:
   *
   * - Find the winding direction and even-odd number,
   * - If the two disagree, reverse the contour, inplace.
   */

  if (!num_endpoints)
    return false;

  if (num_endpoints < 3) {
//    abort (); // XXX remove this after testing lots of fonts?
    return false; // Need at least two arcs
  }
  if (Point (endpoints[0].p) != Point (endpoints[num_endpoints-1].p)) {
    abort (); // XXX remove this after testing lots of fonts?
    return false; // Need a closed contour
   }

  if (!!inverse ^
      winding (endpoints, num_endpoints) ^
      even_odd (endpoints, num_endpoints, all_endpoints, all_num_endpoints))
  {
    glyphy_outline_reverse (endpoints, num_endpoints);
    return true;
  }

  return false;
}

/* Returns true if outline was modified */
glyphy_bool_t
glyphy_outline_winding_from_even_odd (glyphy_arc_endpoint_t *endpoints,
				      unsigned int           num_endpoints,
				      glyphy_bool_t          inverse)
{
  /*
   * Algorithm:
   *
   * - Process one contour at a time.
   */

  unsigned int start = 0;
  glyphy_bool_t ret = false;
  for (unsigned int i = 1; i < num_endpoints; i++) {
    const glyphy_arc_endpoint_t &endpoint = endpoints[i];
    if (endpoint.d == INFINITY) {
      ret = ret || process_contour (endpoints + start, i - start, endpoints, num_endpoints, inverse);
      start = i;
    }
  }
  ret = ret || process_contour (endpoints + start, num_endpoints - start, endpoints, num_endpoints, inverse);
  return ret;
}
