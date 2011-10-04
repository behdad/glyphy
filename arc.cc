/*
 * Copyright © 2011  Google, Inc.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Behdad Esfahbod, Maysum Panju
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <deque>

#include "geometry.hh"
#include "cairo-helper.hh"
#include "sample-curves.hh"
#include "bezier-arc-approximation.hh"


using namespace std;
using namespace Geometry;
using namespace CairoHelper;
using namespace SampleCurves;
using namespace BezierArcApproximation;

typedef Vector<Coord> vector_t;
typedef Point<Coord> point_t;
typedef Line<Coord> line_t;
typedef Circle<Coord, Scalar> circle_t;
typedef Arc<Coord, Scalar> arc_t;
typedef Bezier<Coord> bezier_t;


static void
scale_and_translate (cairo_t *cr, const bezier_t &b)
{
  double cx1, cy1, cx2, cy2;
  cairo_clip_extents (cr, &cx1, &cy1, &cx2, &cy2);
  cairo_translate (cr, (cx1 + cx2) * .5, (cy1 + cy2) * .5);

  cairo_curve (cr, b);

  double px1, py1, px2, py2;
  cairo_path_extents (cr, &px1, &py1, &px2, &py2);
  cairo_new_path (cr);

  double scale = .8 / std::max ((px2 - px1) / (cx2 - cx1), (py2 - py1) / (cy2 - cy1));
  cairo_scale (cr, scale, -scale);
  cairo_set_line_width (cr, cairo_get_line_width (cr) / scale);

  cairo_translate (cr, -(px1 + px2) * .5, -(py1 + py2) * .5);
}

static void
demo_curve (cairo_t *cr, const bezier_t &b)
{
  cairo_save (cr);
  cairo_set_line_width (cr, 3);

  scale_and_translate (cr, b);

  cairo_set_source_rgb (cr, 1, 0, 0);
  cairo_demo_curve (cr, b);

  typedef MaxDeviationApproximatorExact MaxDev;
  typedef BezierArcErrorApproximatorBehdad<MaxDev> BezierArcError;
  typedef BezierArcApproximatorMidpointTwoPart<BezierArcError> BezierArcApproximatorBehdad;
  typedef BezierArcsApproximatorSpring<BezierArcApproximatorBehdad> SpringBehdad;

  double tolerance = 0.00001;
  double e;
  static std::vector<Arc<Coord, Scalar> > &arcs = SpringBehdad::approximate_bezier_with_arcs (b, tolerance, &e);

  printf ("Num arcs %d; Approximation error %g; Tolerance %g; Percentage %g; %s\n",
	  (int) arcs.size (), e, tolerance, round (100 * e / tolerance), e <= tolerance ? "PASS" : "FAIL");

  cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 1.0);
  for (unsigned int i = 0; i < arcs.size (); i++)
    cairo_demo_arc (cr, arcs[i]);

  cairo_restore (cr);
}

int main (int argc, char **argv)
{
  cairo_t *cr;
  char *filename;
  cairo_status_t status;
  cairo_surface_t *surface;

  if (argc != 2) {
    fprintf (stderr, "Usage: arc OUTPUT_FILENAME\n");
    return 1;
  }

  filename = argv[1];

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					1400, 800);
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);

  demo_curve (cr, sample_curve_skewed ());
//  demo_curve (cr, sample_curve_riskus_simple ());
//  demo_curve (cr, sample_curve_riskus_complicated ());
//  demo_curve (cr, sample_curve_riskus_complicated2 ());

  cairo_destroy (cr);

  status = cairo_surface_write_to_png (surface, filename);
  cairo_surface_destroy (surface);

  if (status != CAIRO_STATUS_SUCCESS) {
    fprintf (stderr, "Could not save png to '%s'\n", filename);
    return 1;
  }

  return 0;
}
