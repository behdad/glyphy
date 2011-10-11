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

#include <math.h>

#ifndef GEOMETRY_HH
#define GEOMETRY_HH

namespace Geometry {

typedef double Coord;
typedef double Scalar;

template <typename Type> struct Pair;
template <typename Coord> struct Vector;
template <typename Coord> struct SignedVector;
template <typename Coord> struct Point;
template <typename Coord> struct Line;
template <typename Coord, typename Scalar> struct Circle;
template <typename Coord, typename Scalar> struct Arc;
template <typename Coord> struct Bezier;


template <typename Type>
struct Pair {
  typedef Type ElementType;

  inline Pair (const Type &first_, const Type &second_) : first (first_), second (second_) {}

  Type first, second;
};

template <typename Coord>
struct Vector {
  typedef Coord CoordType;
  typedef Scalar ScalarType;
  typedef Point<Coord> PointType;

  inline Vector (Coord dx_, Coord dy_) : dx (dx_), dy (dy_) {};
  inline explicit Vector (Point<Coord> p) : dx (p.x), dy (p.y) {};

  inline bool operator == (const Vector<Coord> &v) const;
  inline bool operator != (const Vector<Coord> &v) const;
  inline const Vector<Coord> operator- (void) const;
  inline Vector<Coord>& operator+= (const Vector<Coord> &v);
  inline Vector<Coord>& operator-= (const Vector<Coord> &v);
  template <typename Scalar>
  inline Vector<Coord>& operator*= (const Scalar &s);
  template <typename Scalar>
  inline Vector<Coord>& operator/= (const Scalar &s);
  inline const Vector<Coord> operator+ (const Vector<Coord> &v) const;
  inline const Vector<Coord> operator- (const Vector<Coord> &v) const;
  template <typename Scalar>
  inline const Vector<Coord> operator* (const Scalar &s) const;
  template <typename Scalar>
  inline const Vector<Coord> operator/ (const Scalar &s) const;
  inline const Scalar operator* (const Vector<Coord> &v) const; /* dot product */
  inline const Point<Coord> operator+ (const Point<Coord> &p) const;

  inline bool is_nonzero (void) const;
  inline const Scalar len (void) const;
  inline const Vector<Coord> normalized (void) const;
  inline const Vector<Coord> perpendicular (void) const;
  inline const Vector<Coord> normal (void) const; /* perpendicular().normalized() */
  inline const Scalar angle (void) const;

  inline const Vector<Coord> rebase (const Vector<Coord> &bx, const Vector<Coord> &by) const;
  inline const Vector<Coord> rebase (const Vector<Coord> &bx) const;

  Coord dx, dy;
};

template <typename Coord>
struct SignedVector : Vector<Coord> {
  inline SignedVector (const Vector<Coord> &v, bool negative_) : Vector<Coord> (v), negative (negative_) {};

  inline bool operator == (const SignedVector<Coord> &v) const;
  inline bool operator != (const SignedVector<Coord> &v) const;
  inline const SignedVector<Coord> operator- (void) const;

  bool negative;
};

template <typename Coord>
struct Point {
  typedef Coord CoordType;
  typedef Scalar ScalarType;
  typedef Vector<Coord> VectorType;

  inline Point (Coord x_, Coord y_) : x (x_), y (y_) {};
  inline explicit Point (Vector<Coord> v) : x (v.dx), y (v.dy) {};

  inline bool operator == (const Point<Coord> &p) const;
  inline bool operator != (const Point<Coord> &p) const;
  inline Point<Coord>& operator+= (const Vector<Coord> &v);
  inline Point<Coord>& operator-= (const Vector<Coord> &v);
  inline const Point<Coord> operator+ (const Vector<Coord> &v) const;
  inline const Point<Coord> operator- (const Vector<Coord> &v) const;
  inline const Vector<Coord> operator- (const Point<Coord> &p) const;
  inline const Point<Coord> operator+ (const Point<Coord> &v) const; /* mid-point! */
  inline const Line<Coord> operator| (const Point<Coord> &p) const; /* segment axis line! */

  inline bool is_finite (void) const;
  inline const Point<Coord> lerp (const Scalar &a, const Point<Coord> &p) const;

  Coord x, y;
};

template <typename Coord>
struct Line {
  typedef Coord CoordType;
  typedef Vector<Coord> VectorType;
  typedef Point<Coord> PointType;

  inline Line (Coord a_, Coord b_, Scalar c_) : n (a_, b_), c (c_) {};
  inline Line (Vector<Coord> n_, Scalar c_) : n (n_), c (c_) {};
  inline Line (const Point<Coord> &p0, const Point<Coord> &p1);

  inline const Point<Coord> operator+ (const Line<Coord> &l) const; /* line intersection! */
  inline const SignedVector<Coord> operator- (const Point<Coord> &p) const; /* shortest vector from point to line */

  inline const Line<Coord> normalized (void) const;
  inline const Vector<Coord> normal (void) const;

  Vector<Coord> n; /* line normal */
  Scalar c; /* n.dx*x + n.dy*y = c */
};

template <typename Coord, typename Scalar>
struct Circle {
  typedef Coord CoordType;
  typedef Scalar ScalarType;
  typedef Point<Coord> PointType;

  inline Circle (const Point<Coord> &c_, const Scalar &r_) : c (c_), r (r_) {};
  inline Circle (const Point<Coord> &p0, const Point<Coord> &p1, const Point<Coord> &p2) :
		 c ((p0|p1) + (p2|p1)), r ((c - p0).len ()) {}

  inline bool operator == (const Circle<Coord, Scalar> &c) const;
  inline bool operator != (const Circle<Coord, Scalar> &c) const;
  inline const SignedVector<Coord> operator- (const Point<Coord> &p) const; /* shortest vector from point to circle */

  Point<Coord> c;
  Scalar r;
};

template <typename Coord, typename Scalar>
struct Arc {
  typedef Coord CoordType;
  typedef Scalar ScalarType;
  typedef Vector<Coord> VectorType;
  typedef Point<Coord> PointType;
  typedef Circle<Coord, Scalar> CircleType;
  typedef Bezier<Coord> BezierType;

  inline Arc (const Point<Coord> &p0_, const Point<Coord> &p1_, const Point<Coord> &pm, bool complement) :
	      p0 (p0_), p1 (p1_),
	      d (p0_ == pm || p1_ == pm ? 0 :
		 tan ((complement ? 0 : M_PI_2) - ((p1_-pm).angle () - (p0_-pm).angle ()) / 2)) {}
  inline Arc (const Point<Coord> &p0_, const Point<Coord> &p1_, const Scalar &d_) :
	      p0 (p0_), p1 (p1_), d (d_) {}
  inline Arc (const Circle<Coord, Scalar> &c, const Scalar &a0, const Scalar &a1) :
	      p0 (c.c + Vector<Coord> (cos(a0),sin(a0)) * c.r),
	      p1 (c.c + Vector<Coord> (cos(a1),sin(a1)) * c.r),
	      d (tan ((a1 - a0) / 4)) {}

  inline bool operator == (const Arc<Coord, Scalar> &a) const;
  inline bool operator != (const Arc<Coord, Scalar> &a) const;

  inline Scalar radius (void) const;
  inline Point<Coord> center (void) const;
  inline Circle<Coord, Scalar> circle (void) const;

  inline Bezier<Coord> approximate_bezier (Scalar *error) const;

  Point<Coord> p0, p1;
  Scalar d; /* Depth */
};

template <typename Coord>
struct Bezier {
  typedef Coord CoordType;
  typedef Scalar ScalarType;
  typedef Vector<Coord> VectorType;
  typedef Point<Coord> PointType;
  typedef Circle<Coord, Scalar> CircleType;

  inline Bezier (const Point<Coord> &p0_, const Point<Coord> &p1_,
		 const Point<Coord> &p2_, const Point<Coord> &p3_) :
		 p0 (p0_), p1 (p1_), p2 (p2_), p3 (p3_) {}

  template <typename Scalar>
  inline const Point<Coord> point (const Scalar &t) const;

  template <typename Scalar>
  inline const Vector<Coord> tangent (const Scalar &t) const;

  template <typename Scalar>
  inline const Vector<Coord> d_tangent (const Scalar &t) const;

  template <typename Scalar>
  inline const Scalar curvature (const Scalar &t) const;

  template <typename Scalar>
  inline const Circle<Coord, Scalar> osculating_circle (const Scalar &t) const;

  template <typename Scalar>
  inline const Pair<Bezier<Coord> > split (const Scalar &t) const;

  inline const Pair<Bezier<Coord> > halve (void) const;

  inline const Bezier<Coord> segment (const Scalar &t0, const Scalar &t1) const;

  Point<Coord> p0, p1, p2, p3;
};


/* Implementations */

/* Vector */

template <typename Coord>
inline bool Vector<Coord>::operator == (const Vector<Coord> &v) const {
  return dx == v.dx && dy == v.dy;
}
template <typename Coord>
inline bool Vector<Coord>::operator != (const Vector<Coord> &v) const {
  return !(*this == v);
}
template <typename Coord>
inline const Vector<Coord> Vector<Coord>::operator- (void) const {
  return Vector<Coord> (-dx, -dy);
}
template <typename Coord>
inline Vector<Coord>& Vector<Coord>::operator+= (const Vector<Coord> &v) {
  dx += v.dx;
  dy += v.dy;
  return *this;
}
template <typename Coord>
inline Vector<Coord>& Vector<Coord>::operator-= (const Vector<Coord> &v) {
  dx -= v.dx;
  dy -= v.dy;
  return *this;
}
template <typename Coord> template <typename Scalar>
inline Vector<Coord>& Vector<Coord>::operator*= (const Scalar &s) {
  dx *= s;
  dy *= s;
  return *this;
}
template <typename Coord> template <typename Scalar>
inline Vector<Coord>& Vector<Coord>::operator/= (const Scalar &s) {
  dx /= s;
  dy /= s;
  return *this;
}
template <typename Coord>
inline const Vector<Coord> Vector<Coord>::operator+ (const Vector<Coord> &v) const {
  return Vector<Coord> (*this) += v;
}
template <typename Coord>
inline const Vector<Coord> Vector<Coord>::operator- (const Vector<Coord> &v) const {
  return Vector<Coord> (*this) -= v;
}
template <typename Coord> template <typename Scalar>
inline const Vector<Coord> Vector<Coord>::operator* (const Scalar &s) const {
  return Vector<Coord> (*this) *= s;
}
template <typename Scalar>
inline const Vector<Coord> operator* (const Scalar &s, const Vector<Coord> &v) {
  return v * s;
}
template <typename Coord> template <typename Scalar>
inline const Vector<Coord> Vector<Coord>::operator/ (const Scalar &s) const {
  return Vector<Coord> (*this) /= s;
}
template <typename Coord>
inline const Scalar Vector<Coord>::operator* (const Vector<Coord> &v) const { /* dot product */
  return dx * v.dx + dy * v.dy;
}
template <typename Coord>
inline const Point<Coord> Vector<Coord>::operator+ (const Point<Coord> &p) const {
  return p + *this;
}

template <typename Coord>
inline bool Vector<Coord>::is_nonzero (void) const {
  return dx || dy;
}
template <typename Coord>
inline const Scalar Vector<Coord>::len (void) const {
  return hypot (dx, dy);
}
template <typename Coord>
inline const Vector<Coord> Vector<Coord>::normalized (void) const {
  Scalar d = len ();
  return d ? *this / d : *this;
}
template <typename Coord>
inline const Vector<Coord> Vector<Coord>::perpendicular (void) const {
  return Vector<Coord> (-dy, dx);
}
template <typename Coord>
inline const Vector<Coord> Vector<Coord>::normal (void) const {
  return perpendicular ().normalized ();
}
template <typename Coord>
inline const Scalar Vector<Coord>::angle (void) const {
  return atan2 (dy, dx);
}

template <typename Coord>
inline const Vector<Coord> Vector<Coord>::rebase (const Vector<Coord> &bx,
						  const Vector<Coord> &by) const {
  return Vector<Coord> (*this * bx, *this * by);
}
template <typename Coord>
inline const Vector<Coord> Vector<Coord>::rebase (const Vector<Coord> &bx) const {
  return rebase (bx, bx.perpendicular ());
}


/* SignedVector */

template <typename Coord>
inline bool SignedVector<Coord>::operator == (const SignedVector<Coord> &v) const {
  return Vector<Coord>(*this) == Vector<Coord>(v) && negative == v.negative;
}
template <typename Coord>
inline bool SignedVector<Coord>::operator != (const SignedVector<Coord> &v) const {
  return !(*this == v);
}
template <typename Coord>
inline const SignedVector<Coord> SignedVector<Coord>::operator- (void) const {
  return SignedVector<Coord> (-Vector<Coord>(*this), !negative);
}


/* Point */

template <typename Coord>
inline bool Point<Coord>::operator == (const Point<Coord> &p) const {
  return x == p.x && y == p.y;
}
template <typename Coord>
inline bool Point<Coord>::operator != (const Point<Coord> &p) const {
  return !(*this == p);
}
template <typename Coord>
inline Point<Coord>& Point<Coord>::operator+= (const Vector<Coord> &v) {
  x += v.dx;
  y += v.dy;
  return *this;
}
template <typename Coord>
inline Point<Coord>& Point<Coord>::operator-= (const Vector<Coord> &v) {
  x -= v.dx;
  y -= v.dy;
  return *this;
}
template <typename Coord>
inline const Point<Coord> Point<Coord>::operator+ (const Vector<Coord> &v) const {
  return Point<Coord> (*this) += v;
}
template <typename Coord>
inline const Point<Coord> Point<Coord>::operator- (const Vector<Coord> &v) const {
  return Point<Coord> (*this) -= v;
}
template <typename Coord>
inline const Vector<Coord> Point<Coord>::operator- (const Point<Coord> &p) const {
  return Vector<Coord> (x - p.x, y - p.y);
}

template <typename Coord>
inline const Point<Coord> Point<Coord>::operator+ (const Point<Coord> &p) const { /* mid-point! */
  return *this + (p - *this) / 2;
}
template <typename Coord>
inline const Line<Coord> Point<Coord>::operator| (const Point<Coord> &p) const { /* segment axis line! */
  Vector<Coord> d = p - *this;
  return Line<Coord> (d.dx * 2, d.dy * 2, d * Vector<Coord> (p) + d * Vector<Coord> (*this));
}

template <typename Coord>
inline bool Point<Coord>::is_finite (void) const {
  return isfinite (x) && isfinite (y);
}
template <typename Coord>
inline const Point<Coord> Point<Coord>::lerp (const Scalar &a, const Point<Coord> &p) const {
  return Point<Coord> ((1-a) * x + a * p.x, (1-a) * y + a * p.y);
}


/* Line */
template <typename Coord>
Line<Coord>::Line (const Point<Coord> &p0, const Point<Coord> &p1)
{
  n = (p1 - p0).perpendicular ();
  c = n * Vector<Coord> (p0);
}

template <typename Coord>
inline const Point<Coord> Line<Coord>::operator+ (const Line<Coord> &l) const {
  Scalar det = n.dx * l.n.dy - n.dy * l.n.dx;
  if (!det)
    return Point<Coord> (INFINITY, INFINITY);
  return Point<Coord> ((c * l.n.dy - n.dy * l.c) / det,
		       (n.dx * l.c - c * l.n.dx) / det);
}
template <typename Coord>
inline const SignedVector<Coord> Line<Coord>::operator- (const Point<Coord> &p) const {
  /* shortest vector from point to line */
  Scalar mag = -(n * Vector<Coord> (p) - c) / n.len ();
  return SignedVector<Coord> (n.normalized () * mag, mag < 0);
}
template <typename Coord>
inline const SignedVector<Coord> operator- (const Point<Coord> &p, const Line<Coord> &l) {
  return -(l - p);
}

template <typename Coord>
inline const Line<Coord> Line<Coord>::normalized (void) const {
  Scalar d = n.len ();
  return d ? Line<Coord> (n / d, c / d) : *this;
}
template <typename Coord>
inline const Vector<Coord> Line<Coord>::normal (void) const {
  return n;
}


/* Circle */

template <typename Coord, typename Scalar>
inline bool Circle<Coord, Scalar>::operator == (const Circle<Coord, Scalar> &c_) const {
  return c == c_.c && r == c_.r;
}
template <typename Coord, typename Scalar>
inline bool Circle<Coord, Scalar>::operator != (const Circle<Coord, Scalar> &c) const {
  return !(*this == c);
}
#if 0
template <typename Coord, typename Scalar>
inline const SignedVector<Coord> Circle<Coord, typename Scalar>::operator- (const Point<Coord> &p) const {
  /* shortest vector from point to circle */
  Scalar mag = (c - p);
  return SignedVector<Coord> (n.normalized () * mag, mag < 0);
}
template <typename Coord>
inline const SignedVector<Coord> operator- (const Point<Coord> &p, const Circle<Coord> &l) {
  return -(l - p);
}
#endif


/* Arc */

template <typename Coord, typename Scalar>
inline bool Arc<Coord, Scalar>::operator == (const Arc<Coord, Scalar> &a) const {
  return p0 == a.p0 && p1 == a.p1 && d == a.d;
}
template <typename Coord,  typename Scalar>
inline bool Arc<Coord, Scalar>::operator != (const Arc<Coord, Scalar> &a) const {
  return !(*this == a);
}

template <typename Coord,  typename Scalar>
inline Scalar Arc<Coord, Scalar>::radius (void) const
{
  return fabs ((p1 - p0).len () * (1 + d*d) / (4 * d));
}

template <typename Coord,  typename Scalar>
inline Point<Coord> Arc<Coord, Scalar>::center (void) const
{
  return (p0 + p1) + (p0 - p1).perpendicular () * ((1 - d*d) / (4 * d));
}

template <typename Coord,  typename Scalar>
inline Circle<Coord, Scalar> Arc<Coord, Scalar>::circle (void) const
{
  return Circle<Coord, Scalar> (center (), radius ());
}

template <typename Coord,  typename Scalar>
inline Bezier<Coord> Arc<Coord, Scalar>::approximate_bezier (Scalar *error) const {
  if (error)
    *error = (p1 - p0).len () * pow (fabs (d), 5) / (54 * (1 + d*d));

  Point<Coord> p0s = p0 + ((1 - d*d) / 3) * (p1 - p0) + (2 * d / 3) * (p1 - p0).perpendicular ();
  Point<Coord> p1s = p1 + ((1 - d*d) / 3) * (p0 - p1) + (2 * d / 3) * (p1 - p0).perpendicular ();

  return Bezier<Coord> (p0, p0s, p1s, p1);
}


/* Bezier */

template <typename Coord> template <typename Scalar>
inline const Point<Coord> Bezier<Coord>::point (const Scalar &t) const {
  Point<Coord> p01 = p0.lerp (t, p1);
  Point<Coord> p12 = p1.lerp (t, p2);
  Point<Coord> p23 = p2.lerp (t, p3);
  Point<Coord> p012 = p01.lerp (t, p12);
  Point<Coord> p123 = p12.lerp (t, p23);
  Point<Coord> p0123 = p012.lerp (t, p123);
  return p0123;
}

template <typename Coord> template <typename Scalar>
inline const Vector<Coord> Bezier<Coord>::tangent (const Scalar &t) const
{
  double t_2_0 = t * t;
  double t_0_2 = (1 - t) * (1 - t);

  double _1__4t_1_0_3t_2_0 = 1 - 4 * t + 3 * t_2_0;
  double _2t_1_0_3t_2_0    =     2 * t - 3 * t_2_0;

  return Vector<Coord> (-3 * p0.x * t_0_2
			+3 * p1.x * _1__4t_1_0_3t_2_0
			+3 * p2.x * _2t_1_0_3t_2_0
			+3 * p3.x * t_2_0,
			-3 * p0.y * t_0_2
			+3 * p1.y * _1__4t_1_0_3t_2_0
			+3 * p2.y * _2t_1_0_3t_2_0
			+3 * p3.y * t_2_0);
}

template <typename Coord> template <typename Scalar>
inline const Vector<Coord> Bezier<Coord>::d_tangent (const Scalar &t) const {
  return Vector<Coord> (6 * ((-p0.x + 3*p1.x - 3*p2.x + p3.x) * t + (p0.x - 2*p1.x + p2.x)),
			6 * ((-p0.y + 3*p1.y - 3*p2.y + p3.y) * t + (p0.y - 2*p1.y + p2.y)));
}

template <typename Coord> template <typename Scalar>
inline const Scalar Bezier<Coord>::curvature (const Scalar &t) const {
  Vector<Coord> dpp = tangent (t).perpendicular ();
  Vector<Coord> ddp = d_tangent (t);
  /* normal vector len squared */
  double len = dpp.len ();
  double curvature = (dpp * ddp) / (len * len * len);
}

template <typename Coord> template <typename Scalar>
inline const Circle<Coord, Scalar> Bezier<Coord>::osculating_circle (const Scalar &t) const {
  Vector<Coord> dpp = tangent (t).perpendicular ();
  Vector<Coord> ddp = d_tangent (t);
  /* normal vector len squared */
  double len = dpp.len ();
  double curvature = (dpp * ddp) / (len * len * len);
  return Circle<Coord, Scalar> (point (t) + dpp.normalized () / curvature, 1 / curvature);
}

template <typename Coord> template <typename Scalar>
inline const Pair<Bezier<Coord> > Bezier<Coord>::split (const Scalar &t) const {
  Point<Coord> p01 = p0.lerp (t, p1);
  Point<Coord> p12 = p1.lerp (t, p2);
  Point<Coord> p23 = p2.lerp (t, p3);
  Point<Coord> p012 = p01.lerp (t, p12);
  Point<Coord> p123 = p12.lerp (t, p23);
  Point<Coord> p0123 = p012.lerp (t, p123);
  return Pair<Bezier<Coord> > (Bezier<Coord> (p0, p01, p012, p0123),
			       Bezier<Coord> (p0123, p123, p23, p3));
}

template <typename Coord>
inline const Pair<Bezier<Coord> > Bezier<Coord>::halve (void) const
{
  Point<Coord> p01 = p0 + p1;
  Point<Coord> p12 = p1 + p2;
  Point<Coord> p23 = p2 + p3;
  Point<Coord> p012 = p01 + p12;
  Point<Coord> p123 = p12 + p23;
  Point<Coord> p0123 = p012 + p123;
  return Pair<Bezier<Coord> > (Bezier<Coord> (p0, p01, p012, p0123),
			       Bezier<Coord> (p0123, p123, p23, p3));
}

template <typename Coord>
inline const Bezier<Coord> Bezier<Coord>::segment (const Scalar &t0, const Scalar &t1) const
{
  if (fabs (t0 - t1) < 1e-6) {
    Point<Coord> p = point (t0);
    return Bezier<Coord> (p, p, p, p);
  }
  return split (t0).second.split ((t1 - t0) / (1 - t0)).first;
}

} /* namespace Geometry */

#endif
