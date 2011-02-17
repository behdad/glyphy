/* Example code to show how to use pangocairo to render text
 * projected on a path.
 *
 *
 * Written by Behdad Esfahbod, 2006..2007
 *
 * Permission to use, copy, modify, distribute, and sell this example
 * for any purpose is hereby granted without fee.
 * It is provided "as is" without express or implied warranty.
 */

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <pango/pangocairo.h>

typedef int bool;
#define false FALSE
#define true TRUE

typedef struct { double x, y; } point_t;
typedef struct { double a, b, c; } line_t; /* a*x + b*y = c */

static void
bezier_derivatives (double t,
		    point_t p0,
		    point_t p1,
		    point_t p2,
		    point_t p3,
		    point_t *p,
		    point_t *dp,
		    point_t *ddp
		    )
{
  double t_1_0, t_0_1;
  double t_2_0, t_0_2;
  double t_3_0, t_2_1, t_1_2, t_0_3;
  double _1__4t_1_0_3t_2_0, _2t_1_0_3t_2_0;

  t_1_0 = t;
  t_0_1 = 1 - t;

  t_2_0 = t_1_0 * t_1_0; /*      t  *      t  */
  t_0_2 = t_0_1 * t_0_1; /* (1 - t) * (1 - t) */

  t_3_0 = t_2_0 * t_1_0; /*      t  *      t  *      t  */
  t_2_1 = t_2_0 * t_0_1; /*      t  *      t  * (1 - t) */
  t_1_2 = t_1_0 * t_0_2; /*      t  * (1 - t) * (1 - t) */
  t_0_3 = t_0_1 * t_0_2; /* (1 - t) * (1 - t) * (1 - t) */

  _1__4t_1_0_3t_2_0 = 1 - 4 * t_1_0 + 3 * t_2_0;
  _2t_1_0_3t_2_0    =     2 * t_1_0 - 3 * t_2_0;

  /* Bezier polynomial */
  p->x = p0.x * t_0_3
   + 3 * p1.x * t_1_2
   + 3 * p2.x * t_2_1
   +     p3.x * t_3_0;
  p->y = p0.y * t_0_3
   + 3 * p1.y * t_1_2
   + 3 * p2.y * t_2_1
   +     p3.y * t_3_0;

  /* Bezier gradient */
  dp->x = -3 * p0.x * t_0_2
	 + 3 * p1.x * _1__4t_1_0_3t_2_0
	 + 3 * p2.x * _2t_1_0_3t_2_0
	 + 3 * p3.x * t_2_0;
  dp->y = -3 * p0.y * t_0_2
	 + 3 * p1.y * _1__4t_1_0_3t_2_0
	 + 3 * p2.y * _2t_1_0_3t_2_0
	 + 3 * p3.y * t_2_0;

  /* Bezier second derivatives */
  ddp->x = 6 * ( (-p0.x+3*p1.x-3*p2.x+p3.x) * t + (p0.x-2*p1.x+p2.x));
  ddp->y = 6 * ( (-p0.y+3*p1.y-3*p2.y+p3.y) * t + (p0.y-2*p1.y+p2.y));
}

static void
points_difference (point_t A, point_t B, point_t *d)
{
  d->x = B.x - A.x;
  d->y = B.y - A.y;
}

static double
points_distance (point_t A, point_t B)
{
  point_t d;
  points_difference (A, B, &d);
  return hypot (d.x, d.y);
}

static void
vector_normal (point_t v, point_t *n)
{
  n->x = -v.y;
  n->y =  v.x;
}

static void
segments_to_line (point_t p0, point_t p1, line_t *l)
{
  point_t d, n;
  points_difference (p0, p1, &d);
  vector_normal (d, &n);
  l->a = n.x;
  l->b = n.y;
  l->c = n.x * p0.x + n.y * p0.y;
}

static bool
lines_intersection (line_t l0, line_t l1, point_t *p)
{
  double det = l0.a * l1.b - l1.a * l0.b;

  if (!det) {
    p->x = p->y = INFINITY;
    return false;
  }

  p->x = (l0.c * l1.b - l1.c * l0.b) / det;
  p->y = (l0.a * l1.c - l1.a * l0.c) / det;
  return true;
}

static bool
segments_intersection (point_t p0,
		       point_t p1,
		       point_t p2,
		       point_t p3,
		       point_t *p)
{
  line_t l0, l1;

  segments_to_line (p0, p1, &l0);
  segments_to_line (p3, p2, &l1);

  return lines_intersection (l0, l1, p);
}

/* https://secure.wikimedia.org/wikipedia/en/wiki/Incentre#Coordinates_of_the_incenter */
static void
incenter_point (point_t A,
		point_t B,
		point_t C,
		point_t *g)
{
  double a = points_distance (B, C);
  double b = points_distance (C, A);
  double c = points_distance (A, B);
  double P = a + b + c;

  if (P == 0) {
    /* Degenerate case: A = B = C */
    *g = A;
    return;
  }

  g->x = (a * A.x + b * B.x + c * C.x) / P;
  g->y = (a * A.y + b * B.y + c * C.y) / P;
}

void fancy_cairo_stroke (cairo_t *cr);
void fancy_cairo_stroke_preserve (cairo_t *cr);

/* A fancy cairo_stroke[_preserve]() that draws points and control
 * points, and connects them together.
 */
static void
_fancy_cairo_stroke (cairo_t *cr, cairo_bool_t preserve)
{
  int i;
  double line_width;
  cairo_path_t *path;
  cairo_path_data_t *data, current_point;
  const double dash[] = {10, 10};

  cairo_save (cr);
  cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);

  line_width = cairo_get_line_width (cr);
  path = cairo_copy_path (cr);
  cairo_new_path (cr);

  cairo_save (cr);
  cairo_set_line_width (cr, line_width / 3);
  cairo_set_dash (cr, dash, G_N_ELEMENTS (dash), 0);
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
  cairo_set_line_width (cr, line_width / 32);
  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
    case CAIRO_PATH_LINE_TO:
	current_point = data[1];
	break;
    case CAIRO_PATH_CURVE_TO:
	{
#define P(d) (*(point_t *)&(d).point)

	  point_t p0 = P(current_point), p1 = P(data[1]), p2 = P(data[2]), p3 = P(data[3]);
	  point_t v, g;

	  /* XXX */ segments_intersection (p0, p1, p2, p3, &v);

	  incenter_point (p0, v, p3, &g);

	  cairo_save (cr);
	  cairo_move_to (cr, v.x, v.y);
	  cairo_rel_line_to (cr, 0, 0);
	  cairo_move_to (cr, g.x, g.y);
	  cairo_rel_line_to (cr, 0, 0);
	  cairo_set_line_width (cr, line_width * 4);
	  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	  cairo_stroke (cr);
	  cairo_restore (cr);

	  {
	    double t;
	    for (t = 0; t <= 1; t += .01) {
	      point_t p, dp, ddp, nv, cv;
	      double len, curvature;
	      bezier_derivatives (t, P(current_point), P(data[1]), P(data[2]), P(data[3]), &p, &dp, &ddp);

	      /* normal vector len squared */
	      len = sqrt (dp.x*dp.x + dp.y*dp.y);
	      nv.x = -dp.y / len;
	      nv.y =  dp.x / len;

	      curvature = (ddp.y * dp.x - ddp.x * dp.y) / (len*len*len);
	      cv.x = nv.x / curvature;
	      cv.y = nv.y / curvature;

	      cairo_move_to (cr, p.x, p.y);
	      cairo_rel_line_to (cr, cv.x, cv.y);
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

  if (preserve)
    cairo_append_path (cr, path);

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
draw_dream (cairo_t *cr)
{
  cairo_move_to (cr, 50, 650);

  cairo_rel_line_to (cr, 250, 50);
  cairo_rel_curve_to (cr, 250, 50, 600, -50, 600, -250);
  cairo_rel_curve_to (cr, 0, -400, -300, -100, -800, -300);

  cairo_set_line_width (cr, 1.5);
  cairo_set_source_rgba (cr, 0.3, 0.3, 1.0, 0.3);

  fancy_cairo_stroke (cr);
}

static void
draw_wow (cairo_t *cr)
{
  cairo_move_to (cr, 50, 380);
  cairo_scale (cr, 2, 2);

  cairo_rel_curve_to (cr, 50, -50, 150, -50, 200, 0);

  cairo_set_line_width (cr, 2.0);
  cairo_set_source_rgba (cr, 0.3, 1.0, 0.3, 1.0);

  fancy_cairo_stroke (cr);
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
					1000, 800);
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);

  //draw_dream (cr);
  draw_wow (cr);

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
