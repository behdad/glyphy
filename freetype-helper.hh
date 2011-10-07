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

#include "geometry.hh"

#include <algorithm>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#ifndef FREETYPE_HELPER_HH
#define FREETYPE_HELPER_HH

namespace FreeTypeHelper {

using namespace Geometry;

template <typename Arc, class BezierArcsApproximator>
class OutlineArcApproximationGenerator
{
  public:
  typedef Arc ArcType;
  typedef typename ArcType::VectorType VectorType;
  typedef typename ArcType::PointType PointType;
  typedef typename ArcType::BezierType BezierType;

  typedef bool (*Callback) (const Arc &arc, void *closure);

  static bool approximate_glyph (const FT_Outline *outline, Callback callback, void *closure, double tolerance, double *error)
  {
    Closure c = {callback, closure, tolerance, error};

    static const FT_Outline_Funcs outline_funcs = {
        (FT_Outline_MoveToFunc) move_to,
        (FT_Outline_LineToFunc) line_to,
        (FT_Outline_ConicToFunc) conic_to,
        (FT_Outline_CubicToFunc) cubic_to,
        0, /* shift */
        0, /* delta */
    };

    *error = 0;
    return !FT_Outline_Decompose (const_cast <FT_Outline *> (outline), &outline_funcs, &c);
  }

  private:

  struct Closure {
    Callback callback;
    void *closure;
    double tolerance;
    double *error;
    FT_Vector cp; /* current point */
  };

  static int
  move_to (FT_Vector *to, Closure *c)
  {
    c->cp = *to;
    return FT_Err_Ok;
  }

  static int
  line_to (FT_Vector *to, Closure *c)
  {
    bool ret = c->callback (Arc (PointType (c->cp.x, c->cp.y), PointType (to->x, to->y), 0), c->closure);
    c->cp = *to;
    return ret ? FT_Err_Ok : FT_Err_Out_Of_Memory;
  }

  static int
  conic_to (FT_Vector *control, FT_Vector *to, Closure *c)
  {
    PointType pc (control->x, control->y);
    PointType p0 (c->cp.x, c->cp.y);
    PointType p3 (to->x, to->y);
    PointType p1 = p0 + 2/3. * (pc - p0);
    PointType p2 = p3 + 2/3. * (pc - p3);
    BezierType b (p0, p1, p2, p3);
    c->cp = *to;
    return bezier (b, c);
  }

  static int
  cubic_to (FT_Vector *control1, FT_Vector *control2,
	    FT_Vector *to, Closure *c)
  {
    BezierType b (PointType (c->cp.x, c->cp.y),
		  PointType (control1->x, control1->y),
		  PointType (control2->x, control2->y),
		  PointType (to->x, to->y));
    c->cp = *to;
    return bezier (b, c);
  }

  static int
  bezier (const BezierType &b, Closure *c)
  {
    double e;
    std::vector<Arc> &arcs = BezierArcsApproximator::approximate_bezier_with_arcs (b, c->tolerance, &e);
    *c->error = std::max (*c->error, e);

    for (unsigned int i = 0; i < arcs.size (); i++)
      if (!c->callback (arcs[i], c->closure)) {
	delete &arcs;
	return FT_Err_Out_Of_Memory;
      }

    delete &arcs;
    return FT_Err_Ok;
  }
};


} /* namespace FreeTypeHelper */

#endif
