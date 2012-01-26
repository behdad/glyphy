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

#include <glyphy-geometry.hh>
#include <glyphy-arc-bezier.hh>

#include <assert.h>

#include <algorithm>
#include <vector>

#ifndef BEZIER_ARC_APPROXIMATION_HH
#define BEZIER_ARC_APPROXIMATION_HH

namespace GLyphy {
namespace BezierArcApproximation {

using namespace Geometry;
using namespace ArcBezier;

template <class BezierArcApproximator>
class BezierArcsApproximatorSpringSystem
{
  static inline void calc_arcs (const Bezier &b,
				const std::vector<double> &t,
				std::vector<double> &e,
				std::vector<Arc > &arcs,
				double &max_e, double &min_e)
  {
    unsigned int n = t.size () - 1;
    e.resize (n);
    arcs.clear ();
    max_e = 0;
    min_e = INFINITY;
    for (unsigned int i = 0; i < n; i++)
    {
      Bezier segment = b.segment (t[i], t[i + 1]);
      arcs.push_back (BezierArcApproximator::approximate_bezier_with_arc (segment, &e[i]));

      max_e = std::max (max_e, e[i]);
      min_e = std::min (min_e, e[i]);
    }
  }

  static inline void jiggle (const Bezier &b,
			     std::vector<double> &t,
			     std::vector<double> &e,
			     std::vector<Arc > &arcs,
			     double &max_e, double &min_e,
			     double tolerance,
			     unsigned int &n_jiggle)
  {
    unsigned int n = t.size () - 1;
    //fprintf (stderr, "candidate n %d max_e %g min_e %g\n", n, max_e, min_e);
    unsigned int max_jiggle = log2 (n);
    unsigned int s;
    for (s = 0; s <= max_jiggle; s++)
    {
      double total = 0;
      for (unsigned int i = 0; i < n; i++) {
	double l = t[i + 1] - t[i];
	double k_inv = l * pow (e[i], -.3);
	total += k_inv;
	e[i] = k_inv;
      }
      for (unsigned int i = 0; i < n; i++) {
	double k_inv = e[i];
	double l = k_inv / total;
	t[i + 1] = t[i] + l;
      }

      calc_arcs (b, t, e, arcs, max_e, min_e);

      //fprintf (stderr, "n %d jiggle %d max_e %g min_e %g\n", n, s, max_e, min_e);

      n_jiggle++;
      if (max_e < tolerance || (2 * min_e - max_e > tolerance))
	break;
    }
    //if (s == max_jiggle) fprintf (stderr, "JIGGLE OVERFLOW n %d s %d\n", n, s);
  }

  public:
  static std::vector<Arc > &approximate_bezier_with_arcs (const Bezier &b,
									 double tolerance,
									 double *perror,
									 unsigned int max_segments = 1000)
  {
    std::vector<double> t;
    std::vector<double> e;
    std::vector<Arc > &arcs = * new std::vector<Arc >;
    double max_e, min_e;
    unsigned int n_jiggle = 0;

    /* Technically speaking we can bsearch for n. */
    for (unsigned int n = 1; n <= max_segments; n++)
    {
      t.resize (n + 1);
      for (unsigned int i = 0; i <= n; i++)
        t[i] = double (i) / n;

      calc_arcs (b, t, e, arcs, max_e, min_e);

      for (unsigned int i = 0; i < n; i++)
	if (e[i] <= tolerance) {
	  jiggle (b, t, e, arcs, max_e, min_e, tolerance, n_jiggle);
	  break;
	}

      if (max_e <= tolerance)
        break;
    }
    if (perror)
      *perror = max_e;
    //fprintf (stderr, "n_jiggle %d\n", n_jiggle);
    return arcs;
  }
};

template <class BezierArcsApproximator>
class ArcApproximatorOutlineSink
{
  public:

  typedef bool (*Callback) (const Arc &arc, void *closure);

  ArcApproximatorOutlineSink (Callback callback_, void *closure_, double tolerance_)
  : callback (callback_), closure (closure_), tolerance (tolerance_), error (0), p0 (0, 0) {}

  double tolerance;
  double error;
  Callback callback;
  void *closure;
  Point p0; /* current point */

  bool
  move_to (const Point &p)
  {
    p0 = p;
    return true;
  }

  bool
  line_to (const Point &p1)
  {
    bool ret = arc (Arc (p0, p1, 0));
    p0 = p1;
    return ret;
  }

  bool
  conic_to (const Point &p1, const Point &p2)
  {
    return cubic_to (p0 + 2/3. * (p1 - p0),
		     p2 + 2/3. * (p1 - p2),
		     p2);
  }

  bool
  cubic_to (const Point &p1, const Point &p2, const Point &p3)
  {
    bool ret = bezier (Bezier (p0, p1, p2, p3));
    p0 = p3;
    return ret;
  }

  bool
  arc (const Arc &a)
  {
    return callback (a, closure);
  }

  bool
  bezier (const Bezier &b)
  {
    double e;
    std::vector<Arc > &arcs = BezierArcsApproximator
      ::approximate_bezier_with_arcs (b, tolerance, &e);
    error = std::max (error, e);

    bool ret;
    for (unsigned int i = 0; i < arcs.size (); i++)
      if (!(ret = arc (arcs[i])))
        break;

    delete &arcs;
    return ret;
  }
};

typedef BezierArcsApproximatorSpringSystem<BezierArcApproximatorDefault> BezierArcsApproximatorDefault;
typedef ArcApproximatorOutlineSink<BezierArcsApproximatorDefault> ArcApproximatorOutlineSinkDefault;

class ArcAccumulator
{
  public:
  ArcAccumulator (std::vector<Arc> &_arcs) : arcs (_arcs) {}
  static bool callback (const Arc &arc, void *closure)
  {
     ArcAccumulator *acc = static_cast<ArcAccumulator *> (closure);
     acc->arcs.push_back (arc);
     return true;
  }
  std::vector<Arc> &arcs;
};

} /* namespace BezierArcApproxmation */
} /* namespace GLyphy */

#endif /* BEZIER_ARC_APPROXIMATION_HH */
