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
 * Google Author(s): Behdad Esfahbod
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <glib.h>

#include <deque>

#include "geometry.hh"
#include "cairo-helper.hh"
#include "sample-curves.hh"


using namespace std;
using namespace Geometry;
using namespace CairoHelper;
using namespace SampleCurves;

typedef Vector<Coord> vector_t;
typedef Point<Coord> point_t;
typedef Line<Coord> line_t;
typedef Circle<Coord, Scalar> circle_t;
typedef Arc<Coord, Scalar> arc_t;
typedef Bezier<Coord> bezier_t;


/* This is a fast approximation of max_dev(). */
static double
max_dev_approx (double d0, double d1)
{
  d0 = fabs (d0);
  d1 = fabs (d1);
  double e0 = 3./4. * MAX (d0, d1);
  double e1 = 4./9. * (d0 + d1);
  return MIN (e0, e1);
}

/* Returns max(abs(d₀ t (1-t)² + d₁ t² (1-t)) for 0≤t≤1. */
static double
max_dev (double d0, double d1)
{
  double candidates[4] = {0,1};
  unsigned int num_candidates = 2;
  if (d0 == d1)
    candidates[num_candidates++] = .5;
  else {
    double delta = d0*d0 - d0*d1 + d1*d1;
    double t2 = 1. / (3 * (d0 - d1));
    double t0 = (2 * d0 - d1) * t2;
    if (delta == 0)
      candidates[num_candidates++] = t0;
    else if (delta > 0) {
      /* This code can be optimized to avoid the sqrt if the solution
       * is not feasible (ie. lies outside (0,1)).  I have implemented
       * that in cairo-spline.c:_cairo_spline_bound().  Can be reused
       * here.
       */
      double t1 = sqrt (delta) * t2;
      candidates[num_candidates++] = t0 - t1;
      candidates[num_candidates++] = t0 + t1;
    }
  }

  double e = 0;
  for (unsigned int i = 0; i < num_candidates; i++) {
    double t = candidates[i];
    double ee;
    if (t < 0. || t > 1.)
      continue;
    ee = fabs (3 * t * (1-t) * (d0 * (1 - t) + d1 * t));
    e = MAX (e, ee);
  }

  return e;
}

double bezier_arc_error (const bezier_t &b0,
			 const arc_t &a)
{
  double ea;
  bezier_t b1 = a.approximate_bezier (&ea);

  assert (b0.p0 == b1.p0);
  assert (b0.p3 == b1.p3);

//  return max_dev ((b1.p1 - b0.p1).len (), (b1.p2 - b0.p2).len ());

  vector_t v0 = b1.p1 - b0.p1;
  vector_t v1 = b1.p2 - b0.p2;

  vector_t b = (b0.p3 - b0.p0).normal ();
  v0 = v0.rebase (b);
  v1 = v1.rebase (b);

  vector_t v (max_dev (v0.dx, v1.dx),
	      max_dev (v0.dy, v1.dy));

  vector_t b2 = (b1.p3 - b1.p2).rebase (b).normal ();
  vector_t u = v.rebase (b2);

  Scalar c = (b1.p3 - b1.p0).len ();
  double r = fabs (c * (a.d * a.d + 1) / (4 * a.d));
  double eb = sqrt ((r + u.dx) * (r + u.dx) + u.dy * u.dy) - r;

  return ea + eb;
}

static void
demo_curve (cairo_t *cr, const bezier_t &b)
{
  cairo_save (cr);

  double cx1, cy1, cx2, cy2;
  cairo_clip_extents (cr, &cx1, &cy1, &cx2, &cy2);
  cairo_translate (cr, (cx1 + cx2) * .5, (cy1 + cy2) * .5);

  cairo_curve (cr, b);

  double px1, py1, px2, py2;
  cairo_path_extents (cr, &px1, &py1, &px2, &py2);

  double scale = .8 / MAX ((px2 - px1) / (cx2 - cx1), (py2 - py1) / (cy2 - cy1));
  printf ("%g\n", scale);
  cairo_scale (cr, scale, scale);

  cairo_translate (cr, -(px1 + px2) * .5, -(py1 + py2) * .5);
  cairo_new_path (cr);

  cairo_set_source_rgb (cr, 1, 0, 0);
  cairo_set_line_width (cr, 3. / scale);

  cairo_demo_curve (cr, b);

  if (1)
  {
    /* divide the curve into two */
    Pair<bezier_t> pair = b.halve ();
    point_t m = pair.second.p0;

    arc_t a0 (b.p0, m, b.p3, true);
    arc_t a1 (m, b.p3, b.p0, true);
    point_t cc (0,0);
    double e0 = bezier_arc_error (pair.first, a0);
    double e1 = bezier_arc_error (pair.second, a1);
    double e = MAX (e0, e1);

    //double e = bezier_arc_error (b, a);

    printf ("%g %g = %g\n", e0, e1, e);

    arc_t a (b.p0, b.p3, m, true);
    circle_t c = a.circle ();

    {
      double t;
      double e = 0;
      for (t = 0; t <= 1; t += .001) {
	point_t p = b.point (t);
	e = MAX (e, fabs ((c.c - p).len () - c.r));
      }
      printf ("Actual arc max error %g\n", e);
    }

    cairo_save (cr);
    cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 1.0);
    cairo_demo_point (cr, m);
    cairo_demo_arc (cr, a);
    cairo_restore (cr);
  }

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
					1400, 900);
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);

//  demo_curve (cr, sample_curve_skewed ());
//  demo_curve (cr, sample_curve_raskus_simple ());
  demo_curve (cr, sample_curve_raskus_complicated ());
//  demo_curve (cr, sample_curve_raskus_complicated2 ());

  cairo_destroy (cr);

  status = cairo_surface_write_to_png (surface, filename);
  cairo_surface_destroy (surface);

  if (status != CAIRO_STATUS_SUCCESS) {
    fprintf (stderr, "Could not save png to '%s'\n", filename);
    return 1;
  }

  return 0;
}
