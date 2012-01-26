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

#ifndef GLYPHY_H
#define GLYPHY_H

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
glyphy_arc_t
glyphy_arc_from_conventional (glyphy_point_t center,
			      double         radius,
			      double         angle0,
			      double         angle1,
			      glyphy_bool_t  negative);

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
glyph_arc_from_line (glyphy_point_t  p0,
		     glyphy_point_t  p1,
		     glyphy_arc_t   *arc);

void
glyph_arc_from_conic (glyphy_point_t  p0,
		      glyphy_point_t  p1,
		      glyphy_point_t  p2,
		      glyphy_arc_t   *arc,
		      double         *error);

void
glyph_arc_from_cubic (glyphy_point_t  p0,
		      glyphy_point_t  p1,
		      glyphy_point_t  p2,
		      glyphy_point_t  p3,
		      glyphy_arc_t   *arc,
		      double         *error);

void
glyph_arc_to_conic (glyphy_arc_t    arc,
		    glyphy_point_t *p0,
		    glyphy_point_t *p1,
		    glyphy_point_t *p2,
		    double         *error);

void
glyph_arc_to_cubic (glyphy_arc_t    arc,
		    glyphy_point_t *p0,
		    glyphy_point_t *p1,
		    glyphy_point_t *p2,
		    glyphy_point_t *p3,
		    double         *error);


/*
 * Approximate outlines with multiple arcs
 */


typedef struct {
  double x;
  double y;
  double d;
} glyphy_arc_endpoint_t;

typedef struct glyphy_arc_accumulator_t glyphy_arc_accumulator_t;

typedef glyphy_bool_t (*glyphy_arc_endpoint_accumulator_callback_t) (glyphy_arc_accumulator_t *accumulator,
								     glyphy_arc_endpoint_t    endpoint,
								     void                     *user_data);

/* TODO Make this a refcounted opaque type?  Or add destroy_notify? */
typedef struct glyphy_arc_accumulator_t {
  glyphy_bool_t  has_current_point;
  glyphy_point_t current_point;
  unsigned int   num_endpoints;

  double tolerance;
  double error;

  glyphy_arc_endpoint_accumulator_callback_t  callback;
  void                                       *user_data;
} glyphy_arc_accumulator_t;


void
glyphy_arc_accumulator_init (glyphy_arc_accumulator_t *accumulator,
			     double                    tolerance,
			     glyphy_arc_endpoint_accumulator_callback_t callback,
			     void                     *user_data);

glyphy_bool_t
glyphy_arc_accumulator_move_to (glyphy_arc_accumulator_t *accumulator,
				glyphy_point_t p0);

glyphy_bool_t
glyphy_arc_accumulator_line_to (glyphy_arc_accumulator_t *accumulator,
				glyphy_point_t p1);

glyphy_bool_t
glyphy_arc_accumulator_conic_to (glyphy_arc_accumulator_t *accumulator,
				 glyphy_point_t p1,
				 glyphy_point_t p2);

glyphy_bool_t
glyphy_arc_accumulator_cubic_to (glyphy_arc_accumulator_t *accumulator,
				 glyphy_point_t p1,
				 glyphy_point_t p2,
				 glyphy_point_t p3);

glyphy_bool_t
glyphy_arc_accumulator_arc_to (glyphy_arc_accumulator_t *accumulator,
			       glyphy_point_t p1,
			       double         d);

glyphy_bool_t
glyphy_arc_accumulator_close_path (glyphy_arc_accumulator_t *accumulator);



/*
 * Outline extents from arc list
 */


typedef struct {
  double min_x;
  double min_y;
  double max_x;
  double max_y;
} glyphy_extents_t;


glyphy_bool_t
glyphy_arc_list_extents (const glyphy_arc_endpoint_t *endpoints,
			 unsigned int                 num_endpoints,
			 glyphy_extents_t            *extents);



/*
 * Encode an arc outline into binary blob for fast SDF calculation
 */


typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
} glyphy_rgba_t;


glyphy_bool_t
glyphy_arc_list_encode_rgba (const glyphy_arc_endpoint_t *endpoints,
			     unsigned int                 num_endpoints,
			     glyphy_rgba_t               *rgba,
			     unsigned int                 rgba_size,
			     double                       far_away,
			     unsigned int                 avg_fetch_desired,
			     unsigned int                *output_len,
			     unsigned int                *glyph_layout, /* 16bit only */
			     glyphy_extents_t            *extents);



/*
 * Calculate signed-distance-field from (encoded) arc list
 */


typedef struct {
  double dx;
  double dy;
} glyphy_vector_t;


double
glyphy_sdf_from_arc_list (const glyphy_arc_endpoint_t *endpoints,
			  unsigned int                 num_endpoints,
			  glyphy_point_t               p,
			  glyphy_vector_t             *closest /* may be NULL */);

double
glyphy_sdf_from_rgba (const glyphy_rgba_t *rgba,
		      unsigned int         glyph_layout,
		      glyphy_point_t       p,
		      glyphy_vector_t     *closest /* may be NULL */);



/*
 * Shader source code
 */


/* TODO make this enum-based */

const char *
glyphy_common_shader_source (void);

const char *
glyphy_common_shader_source_path (void);

const char *
glyphy_sdf_shader_source (void);

const char *
glyphy_sdf_shader_source_path (void);


#ifdef __cplusplus
}
#endif

#endif /* GLYPHY_H */
