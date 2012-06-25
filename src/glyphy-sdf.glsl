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

#ifndef GLYPHY_TEXTURE1D_FUNC
#define GLYPHY_TEXTURE1D_FUNC glyphy_texture1D_func
#endif
#ifndef GLYPHY_TEXTURE1D_EXTRA_DECLS
#define GLYPHY_TEXTURE1D_EXTRA_DECLS
#endif
#ifndef GLYPHY_TEXTURE1D_EXTRA_ARGS
#define GLYPHY_TEXTURE1D_EXTRA_ARGS
#endif

#ifndef GLYPHY_SDF_TEXTURE1D_FUNC
#define GLYPHY_SDF_TEXTURE1D_FUNC GLYPHY_TEXTURE1D_FUNC
#endif
#ifndef GLYPHY_SDF_TEXTURE1D_EXTRA_DECLS
#define GLYPHY_SDF_TEXTURE1D_EXTRA_DECLS GLYPHY_TEXTURE1D_EXTRA_DECLS
#endif
#ifndef GLYPHY_SDF_TEXTURE1D_EXTRA_ARGS
#define GLYPHY_SDF_TEXTURE1D_EXTRA_ARGS GLYPHY_TEXTURE1D_EXTRA_ARGS
#endif
#ifndef GLYPHY_SDF_TEXTURE1D
#define GLYPHY_SDF_TEXTURE1D(offset) GLYPHY_RGBA (GLYPHY_SDF_TEXTURE1D_FUNC (offset GLYPHY_TEXTURE1D_EXTRA_ARGS))
#endif

#ifndef GLYPHY_MAX_NUM_ENDPOINTS
#define GLYPHY_MAX_NUM_ENDPOINTS 32
#endif

glyphy_arc_list_t
glyphy_arc_list (vec2 p, ivec2 nominal_size GLYPHY_SDF_TEXTURE1D_EXTRA_DECLS)
{
  int cell_offset = glyphy_arc_list_offset (p, nominal_size);
  vec4 arc_list_data = GLYPHY_SDF_TEXTURE1D (cell_offset);
  return glyphy_arc_list_decode (arc_list_data, nominal_size);
}








float
glyphy_sdf (vec2 p, ivec2 nominal_size, out vec2 min_dist_vector GLYPHY_SDF_TEXTURE1D_EXTRA_DECLS)
{
  glyphy_arc_list_t arc_list = glyphy_arc_list (p, nominal_size  GLYPHY_SDF_TEXTURE1D_EXTRA_ARGS);
  
  /* Short-circuits: 0 to 1 arcs near cell*/
  if (arc_list.num_endpoints == 0) {
    /* far-away cell */
    min_dist_vector = vec2 (GLYPHY_INFINITY, GLYPHY_INFINITY);
    return GLYPHY_INFINITY * float(arc_list.side);
  } if (arc_list.num_endpoints == -1) {
    /* single-line */
    float angle = arc_list.line_angle;
    vec2 n = vec2 (cos (angle), sin (angle));
    float dist = dot (p - (vec2(nominal_size) * .5), n) - arc_list.line_distance;
    min_dist_vector = n * dist;
    
    return dist;
  }

  float side = float(arc_list.side);
  float min_dist = GLYPHY_INFINITY;
  glyphy_arc_t closest_arc; 
  
 // float answer = shortest_dist_to_contours (arc_list, p, true, glyphy_arc_endpoint_decode (GLYPHY_SDF_TEXTURE1D (arc_list.offset), nominal_size) )   

  float side2 = float(arc_list.side);
  float min_dist2 = GLYPHY_INFINITY;
  glyphy_arc_t closest_arc2;

  glyphy_arc_endpoint_t endpoint_prev, endpoint;
  
  /* Check arcs on the first contour group. 
   * First contour group is non-empty <=> arc_list.first_contours_length > 0 
   *				      <=> min_dist == GLYPHY_INFINITY after this loop
   */
  endpoint_prev = glyphy_arc_endpoint_decode (GLYPHY_SDF_TEXTURE1D (arc_list.offset), nominal_size);
  for (int i = 1; i < GLYPHY_MAX_NUM_ENDPOINTS; i++)
  {
  
    if (i >= arc_list.num_endpoints || i > arc_list.first_contours_length ) {
      break;
    }
    endpoint = glyphy_arc_endpoint_decode (GLYPHY_SDF_TEXTURE1D (arc_list.offset + i), nominal_size);
    glyphy_arc_t a = glyphy_arc_t (endpoint_prev.p, endpoint.p, endpoint.d);
    endpoint_prev = endpoint;
    if (glyphy_isinf (a.d)) continue; 

    if (glyphy_arc_wedge_contains (a, p))
    {
      float sdist = glyphy_arc_wedge_signed_dist (a, p);
      float udist = abs (sdist) * (1. - GLYPHY_EPSILON);
      if (udist <= min_dist) {
	min_dist = udist;
	side = (sdist <= 0. ? -1. : +1.);
	side = (float(arc_list.side) == -1. ? -1. : side);
	
	/* TODO: Handle case where d=0 (and a has no center). CHECK VECTOR DIRECTION (+/-) */
	if (a.d == 0.) {
	  min_dist_vector = udist * glyphy_unit_vector (glyphy_segment_normal (a));
	  if (distance (p + min_dist_vector, a.p0) > distance (p, a.p0))
	    return 0.;
	}
	else {
	  vec2 center = glyphy_arc_center (a);
	  min_dist_vector = udist * glyphy_unit_vector (center - p) 
	  		* (distance (center, p) < glyphy_arc_radius (a) ? -1. : 1.);
	}
      }
      
    } else {
      float dist0 = distance (p, a.p0);
      float dist1 = distance (p, a.p1);
      float udist = min (dist0, dist1);
      if (udist < min_dist) {
	min_dist = udist;
	side = 0.; /* unsure */
	closest_arc = a;
	if (dist0 < dist1)
	  min_dist_vector = glyphy_unit_vector (a.p0 - p) * udist ;
	else  
	  min_dist_vector = glyphy_unit_vector (a.p1 - p) * udist ;
	  
	  
      } else if (side == 0. && udist == min_dist) {
	/* If this new distance is the same as the current minimum,
	 * compare extended distances.  Take the sign from the arc
	 * with larger extended distance. */
	float old_ext_dist = glyphy_arc_extended_dist (closest_arc, p);
	float new_ext_dist = glyphy_arc_extended_dist (a, p);

	float ext_dist = abs (new_ext_dist) <= abs (old_ext_dist) ?
			 old_ext_dist : new_ext_dist;

#ifdef GLYPHY_SDF_PSEUDO_DISTANCE
	/* For emboldening and stuff: */
	min_dist = abs (ext_dist);
#endif
	side = sign (ext_dist);
	
	
	float dist0 = distance (p, a.p0);
      	float dist1 = distance (p, a.p1);
	if (dist0 < dist1)
	  min_dist_vector = udist * glyphy_unit_vector (a.p0 - p);
	else  
	  min_dist_vector = udist * glyphy_unit_vector (a.p1 - p);
	    
	    
	if (abs (old_ext_dist) < abs (new_ext_dist)) {
	  
	}
	
	 
      }
    }
    
  }  
  if (side == 0. && arc_list.first_contours_length > 0) {
    // Technically speaking this should not happen, but it does.  So try to fix it.
    float ext_dist = glyphy_arc_extended_dist (closest_arc, p);
    side = sign (ext_dist);
  }
    

    
  /* Check arcs on the second contour group. 
   * Second contour group is non-empty IFF arc_list.num_endpoints > arc_list.first_contours_length 
   *				       IFF min_dist2 == GLYPHY_INFINITY after this loop
   */
  vec2 min_dist_vector2;
  endpoint_prev = glyphy_arc_endpoint_decode (GLYPHY_SDF_TEXTURE1D 
  		(arc_list.offset + arc_list.first_contours_length), nominal_size);

  for (int i = arc_list.first_contours_length + 1; i < GLYPHY_MAX_NUM_ENDPOINTS; i++)
  {
  
    if (i >= arc_list.num_endpoints) {
      break;
    }
    endpoint = glyphy_arc_endpoint_decode (GLYPHY_SDF_TEXTURE1D (arc_list.offset + i), nominal_size);
    glyphy_arc_t a = glyphy_arc_t (endpoint_prev.p, endpoint.p, endpoint.d);
    endpoint_prev = endpoint;
    if (glyphy_isinf (a.d)) continue; 

    if (glyphy_arc_wedge_contains (a, p))
    {
      float sdist = glyphy_arc_wedge_signed_dist (a, p);
      float udist = abs (sdist) * (1. - GLYPHY_EPSILON);
      if (udist <= min_dist2) {
	min_dist2 = udist;
	side2 = sdist <= 0. ? -1. : +1.;
	/* TODO: Handle case where d=0 (and a has no center). CHECK VECTOR DIRECTION (+/-) */
	if (a.d == 0.) {
	  min_dist_vector2 = udist * glyphy_unit_vector (glyphy_segment_normal (a));
	  if (distance (p + min_dist_vector2, a.p0) >= distance (p, a.p0))
	    return 0.;
	}
	else {
	  vec2 center = glyphy_arc_center (a);
	  min_dist_vector2 = udist * glyphy_unit_vector (center - p) 
	  		* (distance (center, p) < glyphy_arc_radius (a) ? -1. : 1.);
	}
      }
    } else {
      float dist0 = distance (p, a.p0);
      float dist1 = distance (p, a.p1);
      float udist = min (dist0, dist1);
      if (udist < min_dist2) {
	min_dist2 = udist;
	side2 = 0.; /* unsure */
	closest_arc2 = a;
	if (dist0 < dist1)
	  min_dist_vector2 = glyphy_unit_vector (a.p0 - p) * udist ;
	else   
	  min_dist_vector2 = glyphy_unit_vector (a.p1 - p) * udist ;
      } else if (side2 == 0. && udist == min_dist2) {
	/* If this new distance is the same as the current minimum,
	 * compare extended distances.  Take the sign from the arc
	 * with larger extended distance. */
	float old_ext_dist = glyphy_arc_extended_dist (closest_arc2, p);
	float new_ext_dist = glyphy_arc_extended_dist (a, p);

	float ext_dist = abs (new_ext_dist) <= abs (old_ext_dist) ?
			 old_ext_dist : new_ext_dist;

#ifdef GLYPHY_SDF_PSEUDO_DISTANCE
	/* For emboldening and stuff: */
	min_dist2 = abs (ext_dist);
#endif
	side2 = sign (ext_dist);
	
	if (abs (old_ext_dist) < abs (new_ext_dist)) {
	  float dist0 = distance (p, a.p0);
      	  float dist1 = distance (p, a.p1);
	  if (dist0 < dist1)
	    min_dist_vector2 = (a.p0 - p) * udist / length (a.p0 - p);
	  else  
	    min_dist_vector2 = (a.p1 - p) * udist / length (a.p1 - p);
	}
	
      }
    }
    
    
  }  
  if (side2 == 0.) {
    // Technically speaking this should not happen, but it does.  So try to fix it.
    float ext_dist = glyphy_arc_extended_dist (closest_arc2, p);
    side2 = sign (ext_dist); 
  }  
  
  /** If the two minimum distances are the same, but the sides are different, don't anti-alias. 
    * We are fully contained in one contour region. 
    */
  if (glyphy_iszero (min_dist - min_dist2) && side * side2 == -1.)
    return -1. * GLYPHY_INFINITY;

  /* Update the distance to use as min_dist to outline, based on which contours we are in. */    
  if (side == 0. || (side == 1. && side2 == -1.)) {
    min_dist = min_dist2;
    min_dist_vector = min_dist_vector2;
  }
  else if (side == 1. && side2 == 1.)
    if (min_dist2 < min_dist) {
      min_dist = min_dist2;
      min_dist_vector = min_dist_vector2;
    }
  else if (side == -1. && side2 == -1.)
    if (min_dist < min_dist2) {
      min_dist = min_dist2;
      min_dist_vector = min_dist_vector2;
    }
    
  /* Update side to reflect which side of the overall outline we are at: inside or outside the glyph. */  
  if (side2 < 0. || side == 0.) {
    side = side2;
  } 
  
//  min_dist = length (min_dist_vector);
  
  
//   if (!glyphy_iszero (abs (min_dist) - length (min_dist_vector)))
//     return 0.;  
  return min_dist * side;
}


float
glyphy_point_dist (vec2 p, ivec2 nominal_size GLYPHY_SDF_TEXTURE1D_EXTRA_DECLS)
{
  glyphy_arc_list_t arc_list = glyphy_arc_list (p, nominal_size  GLYPHY_SDF_TEXTURE1D_EXTRA_ARGS);

  float side = float(arc_list.side);
  float min_dist = GLYPHY_INFINITY;

  if (arc_list.num_endpoints == 0)
    return min_dist;

  glyphy_arc_endpoint_t endpoint_prev, endpoint;
  endpoint_prev = glyphy_arc_endpoint_decode (GLYPHY_SDF_TEXTURE1D (arc_list.offset), nominal_size);
  for (int i = 1; i < GLYPHY_MAX_NUM_ENDPOINTS; i++)
  {
    if (i >= arc_list.num_endpoints) {
      break;
    }
    endpoint = glyphy_arc_endpoint_decode (GLYPHY_SDF_TEXTURE1D (arc_list.offset + i), nominal_size);
    if (glyphy_isinf (endpoint.d)) continue;
    min_dist = min (min_dist, distance (p, endpoint.p));
  }
  return min_dist;
}
