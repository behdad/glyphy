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

#ifndef GLYPHY_ARC_H
#define GLYPHY_ARC_H

#ifdef __cplusplus
extern "C" {
#endif



typedef int glyphy_bool_t;



/*
 * Circular arcs
 */


typedef struct {
  double x;
  double y;
} glyphy_point_t;

typedef struct {
  glyphy_point_t p0;
  glyphy_point_t p1;
  double d;
} glyphy_arc_t;


/* Build from a conventional arc representation */
void
glyphy_arc_from_conventional (glyphy_point_t  center,
			      double          radius,
			      double          angle0,
			      double          angle1,
			      glyphy_bool_t   negative,
			      glyphy_arc_t   *arc);

/* Convert to a conventional arc representation */
void
glyphy_arc_to_conventional (glyphy_arc_t    arc,
			    glyphy_point_t *center /* may be NULL */,
			    double         *radius /* may be NULL */,
			    double         *angle0 /* may be NULL */,
			    double         *angle1 /* may be NULL */,
			    glyphy_bool_t  *negative /* may be NULL */);

glyphy_bool_t
glyphy_arc_is_a_line (glyphy_arc_t arc);



/*
 * Approximate single pieces of geometry to/from one arc
 */


void
glyphy_arc_from_line (glyphy_point_t  p0,
		      glyphy_point_t  p1,
		      glyphy_arc_t   *arc);

void
glyphy_arc_from_conic (glyphy_point_t  p0,
		       glyphy_point_t  p1,
		       glyphy_point_t  p2,
		       glyphy_arc_t   *arc,
		       double         *error);

void
glyphy_arc_from_cubic (glyphy_point_t  p0,
		       glyphy_point_t  p1,
		       glyphy_point_t  p2,
		       glyphy_point_t  p3,
		       glyphy_arc_t   *arc,
		       double         *error);

void
glyphy_arc_to_cubic (glyphy_arc_t    arc,
		     glyphy_point_t *p0,
		     glyphy_point_t *p1,
		     glyphy_point_t *p2,
		     glyphy_point_t *p3,
		     double         *error);



#ifdef __cplusplus
}
#endif

#endif /* GLYPHY_ARC_H */
