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
#include <stdint.h>
#include <assert.h>
#include <pango/pangocairo.h>

#include "geometry.hh"


typedef Vector<Coord> vector_t;
typedef Point<Coord> point_t;
typedef Line<Coord> line_t;
typedef Circle<Coord, Scalar> circle_t;
typedef Arc<Coord, Scalar> arc_t;
typedef Bezier<Coord> bezier_t;


void fancy_cairo_stroke (cairo_t *cr);
void fancy_cairo_stroke_preserve (cairo_t *cr);


#define MY_CAIRO_PATH_ARC_TO (CAIRO_PATH_CLOSE_PATH+1)


/* A fancy cairo_stroke[_preserve]() that draws points and control
 * points, and connects them together.
 */
static void
_fancy_cairo_stroke (cairo_t *cr, cairo_bool_t preserve)
{
  int i;
  double line_width;
  cairo_path_t *path;
  cairo_path_data_t *data;

  cairo_save (cr);
  cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);

  line_width = cairo_get_line_width (cr);
  path = cairo_copy_path (cr);
  cairo_new_path (cr);

  cairo_save (cr);
  cairo_set_line_width (cr, line_width / 3);
  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
    case CAIRO_PATH_LINE_TO:
	cairo_move_to (cr, data[1].point.x, data[1].point.y);
	break;
    case CAIRO_PATH_CURVE_TO:
	cairo_line_to (cr, data[1].point.x, data[1].point.y);
	cairo_move_to (cr, data[2].point.x, data[2].point.y);
	cairo_line_to (cr, data[3].point.x, data[3].point.y);
	break;
    case CAIRO_PATH_CLOSE_PATH:
	break;
    default:
	g_assert_not_reached ();
    }
  }
  cairo_stroke (cr);
  cairo_restore (cr);

  cairo_save (cr);
  cairo_set_line_width (cr, line_width * 4);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
	cairo_move_to (cr, data[1].point.x, data[1].point.y);
	break;
    case CAIRO_PATH_LINE_TO:
	cairo_rel_line_to (cr, 0, 0);
	cairo_move_to (cr, data[1].point.x, data[1].point.y);
	break;
    case CAIRO_PATH_CURVE_TO:
	cairo_rel_line_to (cr, 0, 0);
	cairo_move_to (cr, data[1].point.x, data[1].point.y);
	cairo_rel_line_to (cr, 0, 0);
	cairo_move_to (cr, data[2].point.x, data[2].point.y);
	cairo_rel_line_to (cr, 0, 0);
	cairo_move_to (cr, data[3].point.x, data[3].point.y);
	break;
    case CAIRO_PATH_CLOSE_PATH:
	cairo_rel_line_to (cr, 0, 0);
	break;
    default:
	g_assert_not_reached ();
    }
  }
  cairo_rel_line_to (cr, 0, 0);
  cairo_stroke (cr);
  cairo_restore (cr);

  cairo_append_path (cr, path);

  if (preserve)
    cairo_stroke_preserve (cr);
  else
    cairo_stroke (cr);

  cairo_path_destroy (path);

  cairo_restore (cr);
}

/* A fancy cairo_stroke() that draws points and control points, and
 * connects them together.
 */
void
fancy_cairo_stroke (cairo_t *cr)
{
  _fancy_cairo_stroke (cr, FALSE);
}

/* A fancy cairo_stroke_preserve() that draws points and control
 * points, and connects them together.
 */
void
fancy_cairo_stroke_preserve (cairo_t *cr)
{
  _fancy_cairo_stroke (cr, TRUE);
}

static void
print_path_stats (const cairo_path_t *path)
{
  int i;
  cairo_path_data_t *data;
  int lines = 0, curves = 0, arcs = 0;

  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch ((int) data->header.type) {
    case CAIRO_PATH_MOVE_TO:
        break;
    case CAIRO_PATH_LINE_TO:
	lines++;
	break;
    case CAIRO_PATH_CURVE_TO:
	curves++;
	break;
    case MY_CAIRO_PATH_ARC_TO:
	arcs++;
	break;
    case CAIRO_PATH_CLOSE_PATH:
	break;
    default:
	g_assert_not_reached ();
    }
  }
  printf ("%d lines, %d arcs, %d curves\n", lines, arcs, curves);
}

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
demo_curve (cairo_t *cr)
{
  int i;
  double line_width;
  cairo_path_t *path;
  cairo_path_data_t *data, current_point;

  fancy_cairo_stroke_preserve (cr);
  path = cairo_copy_path (cr);
  cairo_new_path (cr);

  print_path_stats (path);

  cairo_save (cr);
  line_width = cairo_get_line_width (cr);
  cairo_set_line_width (cr, line_width / 16);
  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
    case CAIRO_PATH_LINE_TO:
	current_point = data[1];
	break;
    case CAIRO_PATH_CURVE_TO:
	{
#define P(d) (point_t (d.point.x, d.point.y))
	  bezier_t b (P (current_point), P (data[1]), P (data[2]), P (data[3]));
#undef P

	  if (1)
	  {
	    /* divide the curve into two */
	    Pair<bezier_t> pair = b.halve ();
	    point_t m = pair.second.p0;

	    circle_t c (b.p0, m, b.p3);

	    arc_t a0 (b.p0, m, b.p3, true);
	    arc_t a1 (m, b.p3, b.p0, true);
	    double e0 = bezier_arc_error (pair.first, a0);
	    double e1 = bezier_arc_error (pair.second, a1);
	    double e = MAX (e0, e1);

	    //arc_t a (b.p0, b.p3, m, false);
	    //double e = bezier_arc_error (b, a);

	    printf ("%g %g = %g\n", e0, e1, e);

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

	    cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	    cairo_move_to (cr, m.x, m.y);
	    cairo_rel_line_to (cr, 0, 0);
	    cairo_set_line_width (cr, line_width * 4);
	    cairo_stroke (cr);

	    cairo_set_line_width (cr, line_width * 1);
	    cairo_arc (cr, c.c.x, c.c.y, c.r, (b.p0 - c.c).angle (), (b.p3 - c.c).angle ());
	    cairo_stroke (cr);

	    cairo_restore (cr);
	  }

	  {
	    for (double t = 0; t <= 1; t += .05)
	    {
	      point_t p = b.point (t);
	      circle_t cv = b.osculating_circle (t);
	      cairo_move_to (cr, p.x, p.y);
	      cairo_line_to (cr, cv.c.x, cv.c.y);
	    }
	  }
	}
	current_point = data[3];
	break;
    case CAIRO_PATH_CLOSE_PATH:
	break;
    default:
	g_assert_not_reached ();
    }
  }
  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_stroke (cr);
  cairo_restore (cr);

#if 0
  cairo_save (cr);
  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
	cairo_move_to (cr, data[1].point.x, data[1].point.y);
	break;
    case CAIRO_PATH_LINE_TO:
	cairo_line_to (cr, data[1].point.x, data[1].point.y);
	break;
    case CAIRO_PATH_CURVE_TO:
	cairo_curve_to (cr, data[1].point.x, data[1].point.y,
			    data[2].point.x, data[2].point.y,
			    data[3].point.x, data[3].point.y);
	break;
    case CAIRO_PATH_CLOSE_PATH:
	cairo_close_path (cr);
	break;
    default:
	g_assert_not_reached ();
    }
  }
  cairo_stroke (cr);
  cairo_restore (cr);
#endif
  cairo_path_destroy (path);
}

static void
draw_dream (cairo_t *cr)
{
  printf ("SAMPLE: dream line\n");

  cairo_save (cr);
  cairo_new_path (cr);

  cairo_move_to (cr, 50, 650);

  cairo_rel_line_to (cr, 250, 50);
  cairo_rel_curve_to (cr, 250, 50, 600, -50, 600, -250);
  cairo_rel_curve_to (cr, 0, -400, -300, -100, -800, -300);

  cairo_set_line_width (cr, 5);
  cairo_set_source_rgba (cr, 0.3, 1.0, 0.3, 0.3);

  demo_curve (cr);

  cairo_restore (cr);
}

static void
draw_raskus_simple (cairo_t *cr)
{
  printf ("SAMPLE: raskus simple\n");

  cairo_save (cr);
  cairo_new_path (cr);

  cairo_save (cr);
  cairo_translate (cr, -1300, 500);
  cairo_scale (cr, 200, -200);
  cairo_translate (cr, -10, -1);
  cairo_move_to (cr, 16.9753, .7421);
  cairo_curve_to (cr, 18.2203, 2.2238, 21.0939, 2.4017, 23.1643, 1.6148);
  cairo_restore (cr);

  cairo_set_line_width (cr, 2.0);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);

  demo_curve (cr);

  cairo_restore (cr);
}

static void
draw_skewed (cairo_t *cr)
{
  printf ("SAMPLE: skewed\n");

  cairo_save (cr);
  cairo_new_path (cr);

  cairo_move_to (cr, 50, 380);
  cairo_scale (cr, 2, 2);
  cairo_rel_curve_to (cr, 0, -100, 250, -50, 330, 10);

  cairo_set_line_width (cr, 2.0);
  cairo_set_source_rgba (cr, 0.3, 1.0, 0.3, 1.0);

  demo_curve (cr);

  cairo_restore (cr);
}

int main (int argc, char **argv)
{
  cairo_t *cr;
  char *filename;
  cairo_status_t status;
  cairo_surface_t *surface;

  if (argc != 2)
    {
      g_printerr ("Usage: cairotwisted OUTPUT_FILENAME\n");
      return 1;
    }

  filename = argv[1];

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					1400, 1000);
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);

//  draw_skewed (cr);
  draw_raskus_simple (cr);
//  draw_dream (cr);

  cairo_destroy (cr);

  status = cairo_surface_write_to_png (surface, filename);
  cairo_surface_destroy (surface);

  if (status != CAIRO_STATUS_SUCCESS)
    {
      g_printerr ("Could not save png to '%s'\n", filename);
      return 1;
    }

  return 0;
}
