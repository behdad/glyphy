#include <glyphy.hh>

#include <math.h>

#include <assert.h>
#include <string>
#include <list>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

#include "sample-curves.hh"
#include "bezier-arc-approximation.hh"

using namespace std;

namespace GLyphy {

using namespace SampleCurves;
using namespace BezierArcApproximation;


typedef Vector<Coord> vector_t;
typedef Point<Coord> point_t;
typedef Line<Coord> line_t;
typedef Segment<Coord> segment_t;
typedef Circle<Coord, Scalar> circle_t;
typedef Bezier<Coord> bezier_t;

struct rgba_t {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};

#if 0
// Large font size profile
#define MIN_FONT_SIZE 64
#define GRID_SIZE 16
#else
// Small font size profile
#define MIN_FONT_SIZE 20
#define GRID_SIZE 16
#endif

#define GRID_W GRID_SIZE
#define GRID_H GRID_SIZE
#define MAX_TEX_FETCH 6

/* Given a cell, fills the vector closest_arcs with arcs that may be closest to some point in the cell.
 * Uses idea that all close arcs to cell must be ~close to center of cell.
 */
void
closest_arcs_to_cell (point_t p0, point_t p1, /* corners */
		      double grid_size,
		      const vector<arc_t> &arcs,
		      vector<arc_t> &near_arcs,
		      bool &inside_glyph)
{
  inside_glyph = false;
  arc_t current_arc = arcs[0];

  // Find distance between cell center and its closest arc.
  point_t c = p0 + p1;

  SignedVector<Coord> to_arc_min = current_arc - c;
  double min_distance = INFINITY;

  for (int k = 0; k < arcs.size (); k++) {
    current_arc = arcs[k];

    // We can't use squared distance, because sign is important.
    double current_distance = current_arc.distance_to_point (c);

    // If two arcs are equally close to this point, take the sign from the one whose extension is farther away.
    // (Extend arcs using tangent lines from endpoints; this is done using the SignedVector operation "-".)
    if (fabs (fabs (current_distance) - fabs(min_distance)) < 1e-6) {
      SignedVector<Coord> to_arc_current = current_arc - c;
      if (to_arc_min.len () < to_arc_current.len ()) {
        min_distance = fabs (current_distance) * (to_arc_current.negative ? -1 : 1);
      }
    }
    else
      if (fabs (current_distance) < fabs(min_distance)) {
      min_distance = current_distance;
      to_arc_min = current_arc - c;
    }
  }

  inside_glyph = (min_distance > 0);

  // If d is the distance from the center of the square to the nearest arc, then
  // all nearest arcs to the square must be at most [d + s/sqrt(2)] from the center. 
  min_distance =  fabs (min_distance);

  // If d is the distance from the center of the square to the nearest arc, then
  // all nearest arcs to the square must be at most [d + half_diagonal] from the center.
  double half_diagonal = (c - p0).len ();
  double faraway = double (grid_size) / MIN_FONT_SIZE;
  double radius_squared = pow (min_distance + half_diagonal + faraway, 2);
  if (min_distance - half_diagonal <= faraway)
    for (int i = 0; i < arcs.size (); i++) {
      if (arcs[i].squared_distance_to_point (c) <= radius_squared)
        near_arcs.push_back (arcs[i]);
    }
}



/* Bit packing */

#define UPPER_BITS(v,bits,total_bits) ((v) >> ((total_bits) - (bits)))
#define LOWER_BITS(v,bits,total_bits) ((v) & ((1 << (bits)) - 1))
#define MIDDLE_BITS(v,bits,upper_bound,total_bits) (UPPER_BITS (LOWER_BITS (v, upper_bound, total_bits), bits, upper_bound))

const struct rgba_t
arc_encode (double x, double y, double d)
{
  struct rgba_t v;

  // lets do 10 bits for d, and 11 for x and y each 
  unsigned int ix, iy, id;
  ix = lround (x * 4095);
  assert (ix < 4096);
  iy = lround (y * 4095);
  assert (iy < 4096);
#define MAX_D .54 // TODO (0.25?)
  if (isinf (d))
    id = 0;
  else {
    assert (fabs (d) < MAX_D);

    id = lround (d * 127. / MAX_D + 128);

  }
  assert (id < 256);

  v.r = id;
  v.g = LOWER_BITS (ix, 8, 12);
  v.b = LOWER_BITS (iy, 8, 12);
  v.a = ((ix >> 8) << 4) | (iy >> 8);
  return v;
}


struct rgba_t
arclist_encode (unsigned int offset, unsigned int num_points, bool is_inside)
{
  struct rgba_t v;
  v.r = UPPER_BITS (offset, 8, 24);
  v.g = MIDDLE_BITS (offset, 8, 16, 24);
  v.b = LOWER_BITS (offset, 8, 24);
  v.a = LOWER_BITS (num_points, 8, 8);
  if (is_inside && !num_points)
    v.a = 255;
  return v;
}

#if 0
struct atlas_t {
  GLint tex;
  std::hash_map<unsigned int, unsigned int, 

};
#endif

#define IS_INSIDE_NO     0
#define IS_INSIDE_YES    1
#define IS_INSIDE_UNSURE 2

int
arcs_to_texture (std::vector<arc_t> &arcs, int width, int *height,
		 void **buffer)
{
  int grid_min_x =  65535;
  int grid_max_x = -65535;
  int grid_min_y =  65335;
  int grid_max_y = -65535;
  int glyph_width, glyph_height;

  for (int i = 0; i < arcs.size (); i++) {
    grid_min_x = std::min (grid_min_x, (int) floor (arcs[i].leftmost ().x));
    grid_max_x = std::max (grid_max_x, (int) ceil (arcs[i].rightmost ().x));
    grid_min_y = std::min (grid_min_y, (int) floor (arcs[i].lowest ().y));
    grid_max_y = std::max (grid_max_y, (int) ceil (arcs[i].highest ().y));
  }

  glyph_width = grid_max_x - grid_min_x;
  glyph_height = grid_max_y - grid_min_y;

  /* XXX */
  glyph_width = glyph_height = std::max (glyph_width, glyph_height);


  // Make a 2d grid for arc/cell information.
  vector<struct rgba_t> tex_data;

  // near_arcs: Vector of arcs near points in this single grid cell
  vector<arc_t> near_arcs;

  double min_dimension = std::min(glyph_width, glyph_height);
  unsigned int header_length = GRID_W * GRID_H;
  unsigned int offset = header_length;
  tex_data.resize (header_length);
  point_t origin = point_t (grid_min_x, grid_min_y);
  unsigned int saved_bytes = 0;
  unsigned int total_arcs;

  for (int row = 0; row < GRID_H; row++)
    for (int col = 0; col < GRID_W; col++)
    {
      point_t cp0 = origin + vector_t ((col + 0.) * glyph_width / GRID_W, (row + 0.) * glyph_height / GRID_H);
      point_t cp1 = origin + vector_t ((col + 1.) * glyph_width / GRID_W, (row + 1.) * glyph_height / GRID_H);
      near_arcs.clear ();

      bool inside_glyph;
      closest_arcs_to_cell (cp0, cp1, min_dimension, arcs, near_arcs, inside_glyph); 

#define ARC_ENCODE(p, d) \
	arc_encode (((p).x - grid_min_x) / glyph_width, \
		    ((p).y - grid_min_y) / glyph_height, \
		    (d))


      point_t p1 = point_t (0, 0);
      for (unsigned i = 0; i < near_arcs.size (); i++)
      {
        arc_t arc = near_arcs[i];

	if (i == 0 || p1 != arc.p0)
	  tex_data.push_back (ARC_ENCODE (arc.p0, INFINITY));

	tex_data.push_back (ARC_ENCODE (arc.p1, arc.d));
	p1 = arc.p1;
      }

      unsigned int num_endpoints = tex_data.size () - offset;

      /* See if we can fulfill this cell by using already-encoded arcs */
      const struct rgba_t *needle = &tex_data[offset];
      unsigned int needle_len = num_endpoints;
      const struct rgba_t *haystack = &tex_data[header_length];
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
	saved_bytes += needle_len * sizeof (*needle);
      }

      tex_data[row * GRID_W + col] = arclist_encode (offset, num_endpoints, inside_glyph);
      offset = tex_data.size ();

      total_arcs += num_endpoints;

      printf ("%2ld%c ", near_arcs.size (), found ? '.' : 'o');
      if (col == GRID_W - 1)
        printf ("\n");
    }

  printf ("Grid size %dx%d; Used %'ld bytes, saved %'d bytes\n",
	  GRID_W, GRID_H, tex_data.size () * sizeof (tex_data[0]), saved_bytes);
  printf ("Average %g texture accesses\n", 1 + double (total_arcs) / (GRID_W * GRID_H));

  unsigned int tex_len = tex_data.size ();
  unsigned int tex_w = width;
  unsigned int tex_h = (tex_len + tex_w - 1) / tex_w;
  tex_data.resize (tex_w * tex_h);
  *height = tex_h;
  *buffer = new char[tex_data.size() * sizeof(tex_data[0])];
  memcpy(*buffer, &tex_data[0], tex_data.size() * sizeof(tex_data[0]));
  return 0;
}

} /* namespace GLyphy */
