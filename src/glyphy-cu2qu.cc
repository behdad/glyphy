/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 * Cubic-to-quadratic Bézier conversion.
 * Ported from fonttools cu2qu:
 * https://github.com/fonttools/fonttools/blob/main/Lib/fontTools/cu2qu/cu2qu.py
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy.h"

#include <cmath>


/* Maximum approximation error in font units.
 * 0.5 is well below one unit in any reasonable coordinate system. */
#ifndef GLYPHY_CU2QU_TOLERANCE
#define GLYPHY_CU2QU_TOLERANCE 0.5
#endif

/* Maximum subdivision depth.  Each level halves the parameter interval;
 * at depth 10 the sub-curve spans 1/1024 of the original, so the
 * quadratic error (O(dt^4)) is reduced by a factor of ~10^12. */
#define CU2QU_MAX_DEPTH 10


typedef glyphy_point_t Point;


static inline Point
point_lerp (const Point &a, const Point &b, double t)
{
  return Point{a.x + (b.x - a.x) * t,
	       a.y + (b.y - a.y) * t};
}


/*
 * Check whether a cubic error curve stays within `tolerance` of the origin.
 *
 * Given control points (p0, p1, p2, p3) of a cubic that represents the
 * *difference* between two curves, returns true iff the curve never
 * exceeds `tolerance` distance from (0,0).
 *
 * Uses recursive subdivision: evaluate the midpoint, and if it is
 * within tolerance, split and check each half.
 */
static bool
cubic_farthest_fit_inside (Point p0, Point p1, Point p2, Point p3,
			   double tolerance, unsigned depth)
{
  if (hypot (p1.x, p1.y) <= tolerance &&
      hypot (p2.x, p2.y) <= tolerance)
    return true;

  if (depth >= 8)
    return false;

  /* de Casteljau midpoint of the cubic */
  Point mid = {(p0.x + 3 * (p1.x + p2.x) + p3.x) * 0.125,
	       (p0.y + 3 * (p1.y + p2.y) + p3.y) * 0.125};
  if (hypot (mid.x, mid.y) > tolerance)
    return false;

  /* Third of the derivative at the midpoint */
  Point d3 = {(p3.x + p2.x - p1.x - p0.x) * 0.125,
	      (p3.y + p2.y - p1.y - p0.y) * 0.125};

  return cubic_farthest_fit_inside (
    p0,
    point_lerp (p0, p1, 0.5),
    {mid.x - d3.x, mid.y - d3.y},
    mid,
    tolerance, depth + 1
  ) && cubic_farthest_fit_inside (
    mid,
    {mid.x + d3.x, mid.y + d3.y},
    point_lerp (p2, p3, 0.5),
    p3,
    tolerance, depth + 1
  );
}


/*
 * Try to approximate a cubic Bézier with a single quadratic.
 *
 * The quadratic control point is placed at the intersection of the
 * tangent lines at the cubic's endpoints.  Returns true (and fills
 * *q1) if the resulting approximation stays within `tolerance`.
 */
static bool
cubic_approx_quadratic (Point c0, Point c1, Point c2, Point c3,
			double tolerance, Point *q1)
{
  /* Tangent at t=0: c1 − c0,  tangent at t=1: c3 − c2 */
  double ax = c1.x - c0.x, ay = c1.y - c0.y;
  double dx = c3.x - c2.x, dy = c3.y - c2.y;

  /* Perpendicular to start tangent */
  double px = -ay, py = ax;

  /* Denominator: dot(perp, end_tangent) — zero when tangents are parallel */
  double denom = px * dx + py * dy;
  if (fabs (denom) < 1e-12)
    return false;

  /* Parameter h: intersection lies at c2 + h·(c3 − c2) */
  double h = (px * (c0.x - c2.x) + py * (c0.y - c2.y)) / denom;

  q1->x = c2.x + dx * h;
  q1->y = c2.y + dy * h;

  /*
   * Error check.  The quadratic (c0, q1, c3) degree-raised to cubic is
   *   (c0, c0 + ⅔(q1−c0), c3 + ⅔(q1−c3), c3).
   * The error is a cubic with control points (0, err1, err2, 0).
   */
  Point err1 = {c0.x + (q1->x - c0.x) * (2.0 / 3.0) - c1.x,
		c0.y + (q1->y - c0.y) * (2.0 / 3.0) - c1.y};
  Point err2 = {c3.x + (q1->x - c3.x) * (2.0 / 3.0) - c2.x,
		c3.y + (q1->y - c3.y) * (2.0 / 3.0) - c2.y};

  return cubic_farthest_fit_inside ({0, 0}, err1, err2, {0, 0}, tolerance, 0);
}


/*
 * Split a cubic at t = 0.5 (de Casteljau).
 */
static void
split_cubic_half (Point c0, Point c1, Point c2, Point c3,
		  Point out[7])
{
  Point m01  = point_lerp (c0,  c1,  0.5);
  Point m12  = point_lerp (c1,  c2,  0.5);
  Point m23  = point_lerp (c2,  c3,  0.5);
  Point m012 = point_lerp (m01, m12, 0.5);
  Point m123 = point_lerp (m12, m23, 0.5);
  Point mid  = point_lerp (m012, m123, 0.5);

  /* First half:  out[0..3] = c0, m01, m012, mid   */
  /* Second half: out[3..6] = mid, m123, m23, c3    */
  out[0] = c0;   out[1] = m01;  out[2] = m012;
  out[3] = mid;
  out[4] = m123; out[5] = m23;  out[6] = c3;
}


/*
 * Recursively convert a cubic into quadratic segments, emitting each
 * via glyphy_curve_accumulator_conic_to.
 */
static void
cubic_to_quadratics (glyphy_curve_accumulator_t *acc,
		     Point c0, Point c1, Point c2, Point c3,
		     double tolerance, unsigned depth)
{
  /* Try a single quadratic approximation first. */
  Point q1;
  if (cubic_approx_quadratic (c0, c1, c2, c3, tolerance, &q1))
  {
    glyphy_curve_accumulator_conic_to (acc, &q1, &c3);
    return;
  }

  /* At max depth, give up and emit a straight line. */
  if (depth >= CU2QU_MAX_DEPTH)
  {
    glyphy_curve_accumulator_line_to (acc, &c3);
    return;
  }

  /* Subdivide and recurse. */
  Point pts[7];
  split_cubic_half (c0, c1, c2, c3, pts);
  cubic_to_quadratics (acc, pts[0], pts[1], pts[2], pts[3], tolerance, depth + 1);
  cubic_to_quadratics (acc, pts[3], pts[4], pts[5], pts[6], tolerance, depth + 1);
}


/*
 * Public API
 */

void
glyphy_curve_accumulator_cubic_to (glyphy_curve_accumulator_t *acc,
				   const glyphy_point_t *p1,
				   const glyphy_point_t *p2,
				   const glyphy_point_t *p3)
{
  glyphy_point_t c0;
  glyphy_curve_accumulator_get_current_point (acc, &c0);

  /* Degenerate: cubic collapses to a point. */
  if (c0.x == p3->x && c0.y == p3->y &&
      p1->x == p3->x && p1->y == p3->y &&
      p2->x == p3->x && p2->y == p3->y)
    return;

  cubic_to_quadratics (acc, c0, *p1, *p2, *p3, GLYPHY_CU2QU_TOLERANCE, 0);
}
