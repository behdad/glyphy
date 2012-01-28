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
#include "glyphy-rgba.hh"

#include <string.h>
#include <math.h>
#include <assert.h>

#include <vector>

#define GRID_SIZE 16
#define GRID_W GRID_SIZE
#define GRID_H GRID_SIZE

using namespace GLyphy::Geometry;
using namespace GLyphy::RGBA;


static unsigned int /* 16bits */
glyph_layout_encode (unsigned int grid_w,
		     unsigned int grid_h)
{
  assert (0 == (grid_w & ~0xFF));
  assert (0 == (grid_h & ~0xFF));

  return (grid_w << 8) | grid_h;
}

/* Given a cell, fills the vector closest_arcs with arcs that may be closest to some point in the cell.
 * Uses idea that all close arcs to cell must be ~close to center of cell.
 */
static void
closest_arcs_to_cell (Point c0, Point c1, /* corners */
		      double grid_size,
		      double faraway,
		      const glyphy_arc_endpoint_t *endpoints,
		      unsigned int num_endpoints,
		      std::vector<glyphy_arc_endpoint_t> &near_endpoints,
		      bool &inside_glyph)
{
  std::vector<Arc> arcs;
  Point p0 (0, 0);
  for (unsigned int i = 0; i < num_endpoints; i++) {
    const glyphy_arc_endpoint_t &endpoint = endpoints[i];
    if (endpoint.d == INFINITY) {
      p0 = endpoint.p;
      continue;
    }
    Arc arc (p0, endpoint.p, endpoint.d);
    p0 = endpoint.p;

    arcs.push_back (arc);
  }
  std::vector<Arc> near_arcs;


  inside_glyph = false;
  Arc current_arc = arcs[0];

  // Find distance between cell center and its closest arc.
  Point c = c0.midpoint (c1);

  SignedVector to_arc_min = current_arc - c;
  double min_distance = INFINITY;

  for (unsigned int k = 0; k < arcs.size (); k++) {
    current_arc = arcs[k];

    // We can't use squared distance, because sign is important.
    double current_distance = current_arc.distance_to_point (c);

    // If two arcs are equally close to this point, take the sign from the one whose extension is farther away.
    // (Extend arcs using tangent lines from endpoints; this is done using the SignedVector operation "-".)
    if (fabs (current_distance) == fabs (min_distance)) {
      SignedVector to_arc_current = current_arc - c;
      if (to_arc_min.len () < to_arc_current.len ()) {
        min_distance = fabs (current_distance) * (to_arc_current.negative ? -1 : 1);
      }
    }
    else
      if (fabs (current_distance) < fabs (min_distance)) {
      min_distance = current_distance;
      to_arc_min = current_arc - c;
    }
  }

  inside_glyph = (min_distance > 0);

  // If d is the distance from the center of the square to the nearest arc, then
  // all nearest arcs to the square must be at most almost [d + half_diagonal] from the center.
  min_distance =  fabs (min_distance);
  double half_diagonal = (c - c0).len ();
  double radius_squared = pow (min_distance + half_diagonal, 2);
  if (min_distance - half_diagonal <= faraway)
    for (unsigned int i = 0; i < arcs.size (); i++) {
      if (arcs[i].squared_distance_to_point (c) <= radius_squared)
        near_arcs.push_back (arcs[i]);
    }

  Point p1 = Point (0, 0);
  for (unsigned i = 0; i < near_arcs.size (); i++)
  {
    Arc arc = near_arcs[i];

    if (i == 0 || p1 != arc.p0) {
      glyphy_arc_endpoint_t endpoint = {arc.p0, INFINITY};
      near_endpoints.push_back (endpoint);
      p1 = arc.p0;
    }

    glyphy_arc_endpoint_t endpoint = {arc.p1, arc.d};
    near_endpoints.push_back (endpoint);
    p1 = arc.p1;
  }
}


glyphy_bool_t
glyphy_arc_list_encode_rgba (const glyphy_arc_endpoint_t *endpoints,
			     unsigned int                 num_endpoints,
			     glyphy_rgba_t               *rgba,
			     unsigned int                 rgba_size,
			     double                       faraway,
			     double                       avg_fetch_desired,
			     double                      *avg_fetch_achieved,
			     unsigned int                *output_len,
			     unsigned int                *glyph_layout, /* 16bit only will be used */
			     glyphy_extents_t            *pextents /* may be NULL */)
{
  glyphy_extents_t extents;

  glyphy_arc_list_extents (endpoints, num_endpoints, &extents);
  if (pextents)
    *pextents = extents;

  double glyph_width = extents.max_x - extents.min_x;
  double glyph_height = extents.max_y - extents.min_y;

  unsigned int grid_w = GRID_W;
  unsigned int grid_h = GRID_H;

  /* XXX */
  glyph_width = glyph_height = std::max (glyph_width, glyph_height);

  std::vector<glyphy_rgba_t> tex_data;
  std::vector<glyphy_arc_endpoint_t> near_endpoints;

  double min_dimension = std::min(glyph_width, glyph_height);
  unsigned int header_length = grid_w * grid_h;
  unsigned int offset = header_length;
  tex_data.resize (header_length);
  Point origin = Point (extents.min_x, extents.min_y);
  unsigned int total_arcs = 0;

  for (int row = 0; row < grid_h; row++)
    for (int col = 0; col < grid_w; col++)
    {
      Point cp0 = origin + Vector ((col + 0.) * glyph_width / grid_w, (row + 0.) * glyph_height / grid_h);
      Point cp1 = origin + Vector ((col + 1.) * glyph_width / grid_w, (row + 1.) * glyph_height / grid_h);
      near_endpoints.clear ();

      bool inside_glyph;
      closest_arcs_to_cell (cp0, cp1, min_dimension, faraway,
			    endpoints, num_endpoints,
			    near_endpoints,
			    inside_glyph);

#define ARC_ENCODE(E) \
	arc_encode ((E.p.x - extents.min_x) / glyph_width, \
		    (E.p.y - extents.min_y) / glyph_height, \
		    (E.d))

      for (unsigned i = 0; i < near_endpoints.size (); i++) {
        glyphy_arc_endpoint_t &endpoint = near_endpoints[i];
	tex_data.push_back (ARC_ENCODE (endpoint));
      }

      unsigned int current_endpoints = tex_data.size () - offset;

      /* See if we can fulfill this cell by using already-encoded arcs */
      const glyphy_rgba_t *needle = &tex_data[offset];
      unsigned int needle_len = current_endpoints;
      const glyphy_rgba_t *haystack = &tex_data[header_length];
      unsigned int haystack_len = offset - header_length;

      bool found = false;
      if (needle_len)
	while (haystack_len >= needle_len) {
	  /* Trick: we don't care about first endpoint's d value, so skip one
	   * byte in comparison.  This works because arc_encode() packs the
	   * d value in the first byte. */
	  if (0 == memcmp (1 + (const char *) needle,
			   1 + (const char *) haystack,
			   needle_len * sizeof (*needle) - 1)) {
	    found = true;
	    break;
	  }
	  haystack++;
	  haystack_len--;
	}
      if (found) {
	tex_data.resize (offset);
	offset = haystack - &tex_data[0];
      }

      tex_data[row * grid_w + col] = arc_list_encode (offset, current_endpoints, inside_glyph);
      offset = tex_data.size ();

      total_arcs += current_endpoints;
    }

  if (avg_fetch_achieved)
    *avg_fetch_achieved = 1 + double (total_arcs) / (grid_w * grid_h);

  if (tex_data.size () > rgba_size)
    return false;

  memcpy(rgba, &tex_data[0], tex_data.size () * sizeof(tex_data[0]));
  *output_len = tex_data.size ();
  *glyph_layout = glyph_layout_encode (grid_w, grid_h);

  return true;
}
