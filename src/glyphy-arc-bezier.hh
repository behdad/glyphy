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
 * Google Author(s): Behdad Esfahbod, Maysum Panju
 */

#ifndef GLYPHY_ARC_BEZIER_HH
#define GLYPHY_ARC_BEZIER_HH

#include "glyphy-common.hh"
#include "glyphy-geometry.hh"

namespace GLyphy {
namespace ArcBezier {

using namespace Geometry;

/*
 * Note:
 *
 * Some of the classes here are alternative implementations of the same
 * functionality and not used in the library.  Used for algorithm
 * comparisons.
 */



class MaxDeviationApproximatorFast
{
  public:
  /* Returns upper bound for max(abs(d₀ t (1-t)² + d₁ t² (1-t)) for 0≤t≤1. */
  static double approximate_deviation (double d0, double d1)
  {
    d0 = fabs (d0);
    d1 = fabs (d1);
    double e0 = 3./4. * std::max (d0, d1);
    double e1 = 4./9. * (d0 + d1);
    return std::min (e0, e1);
  }
};

class MaxDeviationApproximatorExact
{
  public:
  /* Returns max(abs(d₀ t (1-t)² + d₁ t² (1-t)) for 0≤t≤1. */
  static double approximate_deviation (double d0, double d1)
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
      e = std::max (e, ee);
    }

    return e;
  }
};




template <class MaxDeviationApproximator>
class BezierBezierErrorApproximatorSimpleMagnitude
{
  public:
  static double approximate_bezier_bezier_error (const Bezier &b0, const Bezier &b1)
  {
    assert (b0.p0 == b1.p0);
    assert (b0.p3 == b1.p3);

    return MaxDeviationApproximator::approximate_deviation ((b1.p1 - b0.p1).len (),
							    (b1.p2 - b0.p2).len ());
  }
};

template <class MaxDeviationApproximator>
class BezierBezierErrorApproximatorSimpleMagnitudeDecomposed
{
  public:
  static double approximate_bezier_bezier_error (const Bezier &b0, const Bezier &b1)
  {
    assert (b0.p0 == b1.p0);
    assert (b0.p3 == b1.p3);

    return hypot (MaxDeviationApproximator::approximate_deviation
		  (b1.p1.x - b0.p1.x, b1.p2.x - b0.p2.x),
		  MaxDeviationApproximator::approximate_deviation
		  (b1.p1.y - b0.p1.y, b1.p2.y - b0.p2.y));
  }
};



template <class BezierBezierErrorApproximator>
class ArcBezierErrorApproximatorViaBezier
{
  public:
  static double approximate_bezier_arc_error (const Bezier &b0, const Arc &a)
  {
    double ea;
    Bezier b1 = a.approximate_bezier (&ea);
    double eb = BezierBezierErrorApproximator::approximate_bezier_bezier_error (b0, b1);
    return ea + eb;
  }
};

class ArcBezierErrorApproximatorSampling
{
  public:
  static double approximate_bezier_arc_error (const Bezier &b, const Arc &a,
					      double step = .001)
  {
    assert (b.p0 == a.p0);
    assert (b.p3 == a.p1);

    Circle c = a.circle ();
    double e = 0;
    for (double t = step; t < 1; t += step)
      e = std::max (e, a.distance_to_point (b.point (t)));
    return e;
  }
};

template <class MaxDeviationApproximator>
class ArcBezierErrorApproximatorBehdad
{
  public:
  static double approximate_bezier_arc_error (const Bezier &b0, const Arc &a)
  {
    assert (b0.p0 == a.p0);
    assert (b0.p3 == a.p1);

    double ea;
    Bezier b1 = a.approximate_bezier (&ea);

    assert (b0.p0 == b1.p0);
    assert (b0.p3 == b1.p3);

    Vector v0 = b1.p1 - b0.p1;
    Vector v1 = b1.p2 - b0.p2;

    Vector b = (b0.p3 - b0.p0).normalized ();
    v0 = v0.rebase (b);
    v1 = v1.rebase (b);

    Vector v (MaxDeviationApproximator::approximate_deviation (v0.dx, v1.dx),
	      MaxDeviationApproximator::approximate_deviation (v0.dy, v1.dy));

    /* Edge cases: If d*d is too close to being 1 default to a weak bound. */
    if (fabs(a.d * a.d - 1) < 1e-4)
      return ea + v.len ();

    /* We made sure that fabs(a.d) != 1 */
    double tan_half_alpha = 2 * fabs (a.d) / (1 - a.d*a.d);
    double tan_v;

    /* If v.dy == 0, perturb just a bit. */
    if (fabs(v.dy) < 1e-6) {
       /* TODO Figure this one out. */
       v.dy = 1e-6;
    }
    tan_v = v.dx / v.dy;
    double eb;
    /* TODO Double check and simplify these checks */
    if (fabs (a.d) < 1e-6 || tan_half_alpha < 0 ||
	(-tan_half_alpha <= tan_v && tan_v <= tan_half_alpha))
      return ea + v.len ();

    double c2 = (b1.p3 - b1.p0).len () / 2;
    double r = c2 * (a.d * a.d + 1) / (2 * fabs (a.d));

    eb = Vector (c2 / tan_half_alpha + v.dy, c2 + v.dx).len () - r;

    return ea + eb;
  }
};



template <class ArcBezierErrorApproximator>
class ArcBezierApproximatorMidpointSimple
{
  public:
  static const Arc approximate_bezier_with_arc (const Bezier &b, double *error)
  {
    Pair<Bezier > pair = b.halve ();
    Point m = pair.second.p0;

    Arc a (b.p0, b.p3, m, false);

    *error = ArcBezierErrorApproximator::approximate_bezier_arc_error (b, a);

    return a;
  }
};

template <class ArcBezierErrorApproximator>
class ArcBezierApproximatorMidpointTwoPart
{
  public:
  static const Arc approximate_bezier_with_arc (const Bezier &b, double *error)
  {
    Pair<Bezier > pair = b.halve ();
    Point m = pair.second.p0;

    Arc a0 (b.p0, m, b.p3, true);
    Arc a1 (m, b.p3, b.p0, true);

    double e0 = ArcBezierErrorApproximator::approximate_bezier_arc_error (pair.first, a0);
    double e1 = ArcBezierErrorApproximator::approximate_bezier_arc_error (pair.second, a1);
    *error = std::max (e0, e1);

    return Arc (b.p0, b.p3, m, false);
  }
};

typedef MaxDeviationApproximatorExact MaxDeviationApproximatorDefault;
typedef ArcBezierErrorApproximatorBehdad<MaxDeviationApproximatorDefault> ArcBezierErrorApproximatorDefault;
typedef ArcBezierApproximatorMidpointTwoPart<ArcBezierErrorApproximatorDefault> ArcBezierApproximatorDefault;

} /* namespace ArcBezier */
} /* namespace GLyphy */

#endif /* GLYPHY_ARC_BEZIER_HH */
