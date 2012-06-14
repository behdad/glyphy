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
 * Google Author(s): Behdad Esfahbod, Maysum Panju, Wojciech Baranowski
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy-common.hh"
#include "glyphy-geometry.hh"

#define GRID_SIZE 24

using namespace GLyphy::Geometry;

typedef struct vertex glyphy_contour_vertex_t;
struct vertex {
  unsigned int start_posn;
  unsigned int end_posn;
  unsigned int index; /* index in the relevant list of contours */
  std::vector<unsigned int> dotted_edges;
  std::vector<unsigned int> solid_edges;
};

#define UPPER_BITS(v,bits,total_bits) ((v) >> ((total_bits) - (bits)))
#define LOWER_BITS(v,bits,total_bits) ((v) & ((1 << (bits)) - 1))

#define MAX_X 4095
#define MAX_Y 4095

static inline glyphy_rgba_t
arc_endpoint_encode (unsigned int ix, unsigned int iy, double d)
{
  glyphy_rgba_t v;

  /* 12 bits for each of x and y, 8 bits for d */
  assert (ix <= MAX_X);
  assert (iy <= MAX_Y);
  unsigned int id;
  if (isinf (d))
    id = 0;
  else {
    assert (fabs (d) <= GLYPHY_MAX_D);
    id = 128 + lround (d * 127 / GLYPHY_MAX_D);
  }
  assert (id < 256);

  v.r = id;
  v.g = LOWER_BITS (ix, 8, 12);
  v.b = LOWER_BITS (iy, 8, 12);
  v.a = ((ix >> 8) << 4) | (iy >> 8);
  return v;
}

static inline glyphy_rgba_t
arc_list_encode (unsigned int first_contours_length, unsigned int offset, unsigned int num_points, int side)
{
  glyphy_rgba_t v;
  v.r = LOWER_BITS (first_contours_length, 7, 8); 	//0; // unused for arc-list encoding
  v.g = UPPER_BITS (offset, 8, 16);
  v.b = LOWER_BITS (offset, 8, 16);
  v.a = LOWER_BITS (num_points, 8, 8);
  if (side < 0 && !num_points)
    v.a = 255;
  return v;
}

static inline glyphy_rgba_t
line_encode (const Line &line)
{
  Line l = line.normalized ();
  double angle = l.n.angle ();
  double distance = l.c;

  int ia = lround (-angle / M_PI * 0x7FFF);
  unsigned int ua = ia + 0x8000;
  assert (0 == (ua & ~0xFFFF));

  int id = lround (distance * 0x1FFF);
  unsigned int ud = id + 0x4000;
  assert (0 == (ud & ~0x7FFF));

  /* Marker for line-encoded */
  ud |= 0x8000;

  glyphy_rgba_t v;
  v.r = ud >> 8;
  v.g = ud & 0xFF;
  v.b = ua >> 8;
  v.a = ua & 0xFF;
  return v;
}


/* Given a cell, fills the vector closest_arcs with arcs that may be closest to some point in the cell.
 * Uses idea that all close arcs to cell must be ~close to center of cell.
 */
static void
closest_arcs_to_cell (Point c0, Point c1, /* corners */
		      double faraway,
		      const glyphy_arc_endpoint_t *endpoints,
		      unsigned int num_endpoints,
		      unsigned int cutoff,
		      unsigned int *num_group_1_arcs,
		      std::vector<glyphy_arc_endpoint_t> &near_endpoints,
		      int *side)
{
  // Find distance between cell center
  Point c = c0.midpoint (c1);

  double min_dist1 = glyphy_sdf_from_arc_list (endpoints, cutoff, &c, NULL);
  double min_dist2 = glyphy_sdf_from_arc_list (endpoints + cutoff, num_endpoints - cutoff, &c, NULL);
  double min_dist = fabs (glyphy_sdf_from_arc_list (endpoints, num_endpoints, &c, NULL));
    
  *side = min_dist1 >= 0 ? +1 : -1;
  if (min_dist2 < 0)
    *side = -1;
  
  std::vector<Arc> near_arcs;

  // If d is the distance from the center of the square to the nearest arc, then
  // all nearest arcs to the square must be at most almost [d + half_diagonal] from the center.
  double half_diagonal = (c - c0).len ();
  double radius_squared = (min_dist + half_diagonal) * (min_dist + half_diagonal);
  unsigned int main_contour_arcs = 0;
  
  if (min_dist - half_diagonal <= faraway && 
      (min_dist1 > -1 * half_diagonal && min_dist2 > -1 * half_diagonal) ) {
    Point p0 (0, 0);
    
    
    for (unsigned int i = 0; i < num_endpoints; i++) {
      const glyphy_arc_endpoint_t &endpoint = endpoints[i];
      if (endpoint.d == GLYPHY_INFINITY) {
	p0 = endpoint.p;
	continue;
      }
      Arc arc (p0, endpoint.p, endpoint.d);
      p0 = endpoint.p;

      if (arc.squared_distance_to_point (c) <= radius_squared) {
        near_arcs.push_back (arc);
        if (i < cutoff) {
          main_contour_arcs++;
        } 
      }
    }
  }
    

  *num_group_1_arcs = main_contour_arcs;
  Point p1 = Point (0, 0);
  for (unsigned i = 0; i < near_arcs.size (); i++)
  {
    Arc arc = near_arcs[i];

    if (i == 0 || p1 != arc.p0) {
      glyphy_arc_endpoint_t endpoint = {arc.p0, GLYPHY_INFINITY};
      near_endpoints.push_back (endpoint);
      p1 = arc.p0;
      if (i < main_contour_arcs)
        (*num_group_1_arcs)++;
    }

    glyphy_arc_endpoint_t endpoint = {arc.p1, arc.d};
    near_endpoints.push_back (endpoint);
    p1 = arc.p1;
  }
  
  
/*  printf ("  Endpoints in Group 1: \n");
  for (int j = 0; j < *num_group_1_arcs; j++)
    printf ("    (%f,%f), d=%f\n", near_endpoints[j].p.x, near_endpoints[j].p.y, near_endpoints[j].d);
  printf ("  Endpoints in Group 2: \n");
  for (int j = *num_group_1_arcs; j < near_endpoints.size ();  j++)
    printf ("    (%f,%f), d=%f\n", near_endpoints[j].p.x, near_endpoints[j].p.y, near_endpoints[j].d);*/
}




/* Returns true if the contour in endpoints[start...end-1] intersects any arc in endpoints[0...start-1]. */
glyphy_bool_t
contour_intersects_contour_list (const glyphy_arc_endpoint_t  *endpoints,
			    	 unsigned int 	  	start,
			    	 unsigned int	  	end)
{
  /* Find the smallest box around this contour. */
  glyphy_extents_t extents;
  glyphy_extents_clear (&extents);

  glyphy_arc_list_extents (endpoints + start, end - start, &extents);
  
  for (unsigned int j = 1; j < start; j++) {
  //    if (endpoints[j].d != GLYPHY_INFINITY) ?????????????????????????????????????????????????????
    const glyphy_arc_endpoint_t ethis2 = endpoints[j - 1];
    const glyphy_arc_endpoint_t enext2 = endpoints[j];    
    Arc current_arc (ethis2.p, enext2.p, enext2.d);
    
    glyphy_bool_t feasible = false;
    if (!iszero (current_arc.d)) {
      Point c = current_arc.center ();
      double r = current_arc.radius ();
      feasible = (c.x - r <= extents.max_x && c.x + r >= extents.min_x && c.y + r >= extents.min_y && c.y - r <= extents.max_y);
    } else {
      double min_x = (current_arc.p0.x < current_arc.p1.x ? current_arc.p0.x : current_arc.p1.x);
      double min_y = (current_arc.p0.y < current_arc.p1.y ? current_arc.p0.y : current_arc.p1.y);
      double max_x = (current_arc.p0.x > current_arc.p1.x ? current_arc.p0.x : current_arc.p1.x);
      double max_y = (current_arc.p0.y > current_arc.p1.y ? current_arc.p0.y : current_arc.p1.y);
      feasible = (min_x <= extents.max_x && max_x >= extents.min_x && max_y >= extents.min_y && min_y <= extents.max_y);
    }

    if (feasible) {
      /* If it seems feasible that this arc-arc pair (one per contour) might intersect, then check carefully. */
      for (unsigned int i = start + 1; i < end; i++) {
        const glyphy_arc_endpoint_t ethis = endpoints[i - 1];
        const glyphy_arc_endpoint_t enext = endpoints[i];    
        Arc a (ethis.p, enext.p, enext.d);
      
        if (current_arc.intersects_arc (a) != Point (GLYPHY_INFINITY, GLYPHY_INFINITY)) {
          printf("Contours intersect.\n");
          return true;      
        }
      }
    }
  } 
  printf("Contours do not intersect.\n");
  return false;
}


/* Returns true if the contour in endpoints[start...end-1] intersects any arc in endpoints[0...start-1]. */
glyphy_bool_t
contours_intersect (const glyphy_arc_endpoint_t    *endpoints,
		    const glyphy_contour_vertex_t  *contour_1,
		    const glyphy_contour_vertex_t  *contour_2)
{
  /* Find the smallest box around these contours. 
   * TODO: Okay, so extents don't *actually* work just yet. */
  glyphy_extents_t extents_1;
  glyphy_extents_clear (&extents_1);
  glyphy_arc_list_extents (endpoints + contour_1->start_posn, contour_1->end_posn - contour_1->start_posn, &extents_1);
  
//  printf("Extents1: (%f,%f) by (%f,%f)\n", extents_1.min_x, extents_1.max_x, extents_1.min_y, extents_1.max_y);
  
  glyphy_extents_t extents_2;
  glyphy_extents_clear (&extents_2);
  glyphy_arc_list_extents (endpoints + contour_2->start_posn, contour_2->end_posn - contour_2->start_posn, &extents_2);
    
//  printf("Extents2: (%f,%f) by (%f,%f)\n", extents_2.min_x, extents_2.max_x, extents_2.min_y, extents_2.max_y);
  
  glyphy_bool_t feasible = false;
  feasible = (extents_1.min_x <= extents_2.max_x && 
  	      extents_1.max_x >= extents_2.min_x && 
  	      extents_1.max_y >= extents_2.min_y && 
  	      extents_1.min_y <= extents_2.max_y);
  	      
  if (!feasible)
    return false;
    
  // If it seems feasible that this arc-arc pair (one per contour) might intersect, then check carefully.
  for (unsigned int j = contour_1->start_posn + 1; j < contour_1->end_posn; j++) {
      const glyphy_arc_endpoint_t ethis1 = endpoints[j - 1];
      const glyphy_arc_endpoint_t enext1 = endpoints[j];    
      Arc a1 (ethis1.p, enext1.p, enext1.d);
        
      for (unsigned int i = contour_2->start_posn + 1; i < contour_2->end_posn; i++) {
        const glyphy_arc_endpoint_t ethis2 = endpoints[i - 1];
        const glyphy_arc_endpoint_t enext2 = endpoints[i];    
        Arc a2 (ethis2.p, enext2.p, enext2.d);
      
        if (a1.intersects_arc (a2) != Point (GLYPHY_INFINITY, GLYPHY_INFINITY)) {
          return true;     
        } 
      } 
  } 
  return false; 
}

/* Rearrange contours into two groups which do not intersect each other, based on a heuristic. 
 * Soon to be deprecated. */
unsigned int
rearrange_contours (const glyphy_arc_endpoint_t *endpoints,
		    unsigned int	  num_endpoints,
		    glyphy_arc_endpoint_t *rearranged_endpoints)
{
  
  if (num_endpoints == 0)
    return 0;
    
  /* Copy first contour unchanged. */
  unsigned int i = 0;
  while (i + 1 < num_endpoints && endpoints[i + 1].d != GLYPHY_INFINITY) {
    rearranged_endpoints [i] = glyphy_arc_endpoint_t (endpoints[i]);
    i++;
  }
  rearranged_endpoints [i] = endpoints [i];
  i++;
  
    
  /* Add new contours one-by-one. 
   * If the new contour intersects a contour at the top of the rearranged list, add it to the bottom.
   * Otherwise, add it to the top.
   */
   unsigned int top = i;
   unsigned int bottom = num_endpoints;   
   unsigned int start = i;
   
   while (top < bottom ) {
   
     while (endpoints[i + 1].d != GLYPHY_INFINITY && i + 1 < num_endpoints ) {
       i++;
     }
     i++;
  
  
     if (contour_intersects_contour_list (endpoints, start, i)) { 
       for (unsigned int j = start; j < i; j++) {
         rearranged_endpoints[bottom - i + j] = endpoints[j];
       }
       bottom = bottom - (i - start);
     }
     else {
       for (unsigned int j = start; j < i; j++) {
         rearranged_endpoints[top + j - start] = endpoints[j]; 
       }
       top = top + (i - start);
     }       
     start = i;
   }
   
  
  return top;
}


void
populate_connected_component (const std::vector<glyphy_contour_vertex_t> contours, 
			      const unsigned int			 current_contour, 
			      std::vector<unsigned int>  		 *connected_contours,
			      bool					 *contours_seen)
{
  if (contours_seen [current_contour])
    return;
  contours_seen [current_contour] = true;
  
  connected_contours->push_back (current_contour);
  printf ("  Just pushed back contour (%d to %d)!\n", contours[current_contour].start_posn, contours[current_contour].end_posn);
  for (unsigned int k = 0; k < contours[current_contour].dotted_edges.size (); k++)
    populate_connected_component (contours, 
    				  contours[contours[current_contour].dotted_edges[k]].index, /* o_O */
    				  connected_contours, contours_seen);
}

void 
assign_contour_levels (const std::vector<glyphy_contour_vertex_t> new_contours,
		       const unsigned int			  current_contour,
		       const unsigned int			  projected_level,
		       int*					  contour_levels)
{
  if (contour_levels [current_contour] != -1)
    return;
    
  contour_levels [current_contour] = projected_level;
  for (unsigned int i = 0; i < new_contours [current_contour].solid_edges.size (); i++)
    assign_contour_levels (new_contours, new_contours [current_contour].solid_edges [i], projected_level + 1, contour_levels);
}     


/* Rearranges contours into two groups that don't intersect, based on a bipartite graph partition. 
 * Still in progress. */
unsigned int
rearrange_contours2 (const glyphy_arc_endpoint_t *endpoints,
		     unsigned int	  num_endpoints,
		     glyphy_arc_endpoint_t *rerearranged_endpoints)
{
  
  if (num_endpoints == 0)
    return 0;
  
  std::vector<glyphy_contour_vertex_t> contours;
  unsigned int previous_index = 0;
    
  /* Create a list of vertices, where each vertex is a contour. Edges are still empty for now. */
  unsigned int i = 0;
  unsigned int num_contours = 0;
   while (i < num_endpoints) {
  
    while (i + 1 < num_endpoints && endpoints[i + 1].d != GLYPHY_INFINITY) {
      i++;
    }    
    i++;
    glyphy_contour_vertex_t current_contour;
  
    current_contour.start_posn = previous_index;
    current_contour.end_posn = i;
    current_contour.index = num_contours;
    current_contour.dotted_edges = std::vector<unsigned int> ();
    current_contour.solid_edges = std::vector<unsigned int> ();
    
    contours.push_back (current_contour);
    num_contours++;
    previous_index = i;     
  }
  
  /* Set up edges for vertices, based on intersections and inclusions. */
  for (unsigned int k = 0; k < num_contours; k++) {
    for (unsigned int j = 0; j < k; j++) {
    
    
      /* If contours intersect, we place a solid edge between them. */
      if (contours_intersect (endpoints, &contours[k], &contours[j])) {
        contours[k].solid_edges.push_back (j);
        contours[j].solid_edges.push_back (k);
      }
      else 
      /** If one contour contains the other, we place a dotted edge between them. 
        * To check if a contour contains another, it is sufficient to check 
        * if contour_1 contains a point from contour_2, or vice versa, since we already
        * know that these contours don't intersect. (For the same reason, we can be sure that
        * the point from the first contour will not lie on the second contour.)
        * Here we can use some code from glyphy-outline::even_odd.
        */
      
      if (!even_odd (endpoints + contours[k].start_posn, 1, 
      		    endpoints + contours[j].start_posn, contours[j].end_posn - contours[j].start_posn) ||
      	  !even_odd (endpoints + contours[j].start_posn, 1, 
      		    endpoints + contours[k].start_posn, contours[k].end_posn - contours[k].start_posn)) {
	contours[k].dotted_edges.push_back (j);
        contours[j].dotted_edges.push_back (k);
      
      }
    }
  }
  
  
    printf("contours_size:%d\n", int (contours.size()));
  
/* Print out a list of contours and the contours they intersect. */
  for (int j = 0; j < contours.size (); j++) {
    printf ("----------------------------------\n");
    printf ("Contour %d spans from %d to %d. ", j, contours[j].start_posn, contours[j].end_posn);
    for (int k = 0; k < contours[j].solid_edges.size (); k++) 
      printf("It intersects contour #%d. ", contours[j].solid_edges[k]);
    printf("\n"); 
    for (int k = 0; k < contours[j].dotted_edges.size (); k++)
      printf("It includes the old contour #%d. ", contours[j].dotted_edges[k]);
    printf("\n"); 
    
    printf("Contour Endpoint List:\n");
    for (unsigned int k = contours[j].start_posn; k < contours[j].end_posn; k++) {
      printf("#%2d. (%f,%f) with d=%f.\n", k, endpoints[k].p.x, endpoints[k].p.y, endpoints[k].d);
    }
  }
  

  
  /* Collapse all dotted edges by "merging" vertices together. 
   * Note: We are assuming quite a bit about the graph structure here.
   * Hopefully it all works in the cases that ever come up. */
  bool contours_seen [contours.size ()];
  // TODO: Is there a better way to initialize the array to all false?
  for (int j = 0; j < contours.size(); j++) {
    contours_seen[j] = false;
    printf("%s", contours_seen[j] ? "O" : ".");
  }
  unsigned int current_end = 0;
  std::vector<glyphy_contour_vertex_t> new_contours;
  unsigned int num_new_contours = 0;
  unsigned int new_endpoint_list_index = 0;
  
  /* For each entry in the vector of contour vertices, make a list of contours that it has a dot-line connection with. */
  std::vector<unsigned int> connected_contours;
  glyphy_arc_endpoint_t rearranged_endpoints [num_endpoints];
  
  
  for (unsigned int j = 0; j < contours.size (); j++) {
    if (contours_seen [j])
      continue;
      
    connected_contours.clear ();
    populate_connected_component (contours, j, &connected_contours, contours_seen);
    
    printf("These contours are connected: ");
    
    /* Create a new vertex representing the merged contours. */
    glyphy_contour_vertex_t merged_contour;
    merged_contour.start_posn = new_endpoint_list_index;    
    merged_contour.index = num_new_contours;
    merged_contour.dotted_edges = std::vector<unsigned int> ();
    merged_contour.solid_edges = std::vector<unsigned int> ();
    
    
    for (unsigned int k = 0; k < connected_contours.size (); k++) {
      printf("(%d to %d); ", contours [connected_contours [k]].start_posn, contours [connected_contours [k]].end_posn);
      
      merged_contour.dotted_edges.push_back (contours [connected_contours [k]].index);
      
      /* Update the old contour vertex to tell us which new vertex it is now part of. */
      contours [connected_contours [k]].index = merged_contour.index;
      
      
      /* Add all the endpoints to the rearranged endpoint array. */
      for (unsigned int m = contours [connected_contours [k]].start_posn; m < contours [connected_contours [k]].end_posn; m++) {
        rearranged_endpoints [new_endpoint_list_index] = endpoints [m];
        new_endpoint_list_index++;
      }
    }  
    merged_contour.end_posn = new_endpoint_list_index;
    new_contours.push_back (merged_contour);
    num_new_contours++; 
    
    printf("JUST MADE NEW CONTOUR #%d.\n", new_contours[num_new_contours-1].index);

  }
  
  for (unsigned int j = 0; j < new_contours.size (); j++) 
    printf ("------\nNew Contour %d has index %d.\n", j, new_contours[j].index);
  
  
  
  /* Merge the lists of solid edges.  TODO This should happen outside, and use new contour references.*/
  for (unsigned int j = 0; j < new_contours.size (); j++) {
    new_contours[j].solid_edges.clear ();
    
    for (unsigned int m = 0; m < new_contours[j].dotted_edges.size (); m++) {
  
      for (unsigned int k = 0; k < contours[new_contours[j].dotted_edges[m]].solid_edges.size (); k++) {
    
        
        unsigned int solid_edge_to_add = contours[contours[new_contours[j].dotted_edges[m]].solid_edges[k]].index;
        printf("    (%d) Trying to add edge %d.\n", contours[new_contours[j].dotted_edges[m]].solid_edges[k], solid_edge_to_add);
        bool is_original_edge = true;
        for (unsigned int existing_edge = 0; existing_edge < new_contours[j].solid_edges.size (); existing_edge++)  {
          if (solid_edge_to_add == new_contours[j].solid_edges [existing_edge]) 
            is_original_edge = false;
   	}
        if (is_original_edge) {
          new_contours[j].solid_edges.push_back (solid_edge_to_add);
          printf("     Success!\n");
        }
      } 
    }
 
    
    printf("Made new contour: index=%d, startposn=%d, endposn=%d.\n", new_contours[j].index, new_contours[j].start_posn, new_contours[j].end_posn);
    for (unsigned int m = 0; m < new_contours[j].dotted_edges.size (); m++)
      printf("Dotted edge to %d! ", new_contours[j].dotted_edges[m]);
    printf("\n");
    for (unsigned int m = 0; m < new_contours[j].solid_edges.size (); m++)
      printf("Solidd edge to %d! ", new_contours[j].solid_edges[m]);
    printf("\n");
  }
  
  
  printf("new_contours_size:%d\n", int (new_contours.size()));
  
/* Print out a list of contours and the contours they intersect. */
  for (int j = 0; j < new_contours.size (); j++) {
    printf ("@@---------------------------------\n");
    printf ("Contour %d spans from %d to %d. ", j, new_contours[j].start_posn, new_contours[j].end_posn);
    for (int k = 0; k < new_contours[j].solid_edges.size (); k++) 
      printf("It intersects contour #%d. ", new_contours[j].solid_edges[k]);
    printf("\n"); 
    for (int k = 0; k < new_contours[j].dotted_edges.size (); k++)
      printf("It includes the old contour #%d. ", new_contours[j].dotted_edges[k]);
    printf("\n"); 
    
 //   printf("Contour Endpoint List:\n");
 //   for (unsigned int k = new_contours[j].start_posn; k < new_contours[j].end_posn; k++) {
 //     printf("#%2d. (%f,%f) with d=%f.\n", k, rearranged_endpoints[k].p.x, rearranged_endpoints[k].p.y, rearranged_endpoints[k].d);
 //   }
  }
  
  /* Time to bipartition the graph, which should contain only solid edges at this point. */
  
  int contour_levels [new_contours.size ()];
  // TODO: Is there a better way to initialize the array to all -1?
  for (int j = 0; j < new_contours.size(); j++) {
    contour_levels [j] = -1;
    printf("%s", contour_levels [j] != -1 ? "O" : ".");
  }
  
  for (unsigned int j = 0; j < new_contours.size (); j++) {
    if (contour_levels [j] != -1)
      continue;
    assign_contour_levels (new_contours, j, 0, contour_levels);      
  }
  
  printf("The contours levels are as follows: \n");
  for (unsigned int j = 0; j < new_contours.size (); j++) {
    printf ("Contour %d: Level %d.\n", j, contour_levels [j]);
  }
  
  
  /* Add new contours one-by-one. 
   * If the new contour has an even level, add it to the top of the list.
   * Otherwise, add it to the bottom.
   */
   unsigned int top = 0;
   unsigned int bottom = num_endpoints;   
   
   for (i = 0; i < new_contours.size (); i++) {
     if (contour_levels [i] % 2 == 0) {
       for (unsigned int j = new_contours [i].start_posn; j < new_contours [i].end_posn; j++) {
         rerearranged_endpoints[top + j - new_contours [i].start_posn] = rearranged_endpoints[j]; 
       }
       top = top + (new_contours [i].end_posn - new_contours [i].start_posn);
     } else {
       for (unsigned int j = new_contours [i].start_posn; j < new_contours [i].end_posn; j++) {
         rerearranged_endpoints[bottom + j - new_contours [i].end_posn] = rearranged_endpoints[j];
       }
       bottom = bottom - (new_contours [i].end_posn - new_contours [i].start_posn);
     }
     
   
   }

   
     
  return top;

}





glyphy_bool_t
glyphy_arc_list_encode_blob (const glyphy_arc_endpoint_t *endpoints,
			     unsigned int                 num_endpoints,
			     glyphy_rgba_t               *blob,
			     unsigned int                 blob_size,
			     double                       faraway,
			     double                       avg_fetch_desired,
			     double                      *avg_fetch_achieved,
			     unsigned int                *output_len,
			     unsigned int                *nominal_width,  /* 8bit */
			     unsigned int                *nominal_height, /* 8bit */
			     glyphy_extents_t            *pextents)
{
  glyphy_extents_t extents;
  glyphy_extents_clear (&extents);

  glyphy_arc_list_extents (endpoints, num_endpoints, &extents);

  if (glyphy_extents_is_empty (&extents)) {
    *pextents = extents;
    if (!blob_size)
      return false;
    *blob = arc_list_encode (0, 0, 0, +1);
    *avg_fetch_achieved = 1;
    *output_len = 1;
    *nominal_width = *nominal_height = 1;
    return true;
  }

  /* Add antialiasing padding */
  extents.min_x -= faraway;
  extents.min_y -= faraway;
  extents.max_x += faraway;
  extents.max_y += faraway;

  double glyph_width = extents.max_x - extents.min_x;
  double glyph_height = extents.max_y - extents.min_y;
  double unit = std::max (glyph_width, glyph_height);

  unsigned int grid_w = GRID_SIZE;
  unsigned int grid_h = GRID_SIZE;

  if (glyph_width > glyph_height) {
    while ((grid_h - 1) * unit / grid_w > glyph_height)
      grid_h--;
    glyph_height = grid_h * unit / grid_w;
    extents.max_y = extents.min_y + glyph_height;
  } else {
    while ((grid_w - 1) * unit / grid_h > glyph_width)
      grid_w--;
    glyph_width = grid_w * unit / grid_h;
    extents.max_x = extents.min_x + glyph_width;
  }

  double cell_unit = unit / std::max (grid_w, grid_h);

  std::vector<glyphy_rgba_t> tex_data;
  std::vector<glyphy_arc_endpoint_t> near_endpoints;

  unsigned int header_length = grid_w * grid_h;
  unsigned int offset = header_length;
  tex_data.resize (header_length);
  Point origin = Point (extents.min_x, extents.min_y);
  unsigned int total_arcs = 0;


  /* Here is where we divide the arc list into two, based on intersecting contours. */  
  glyphy_arc_endpoint_t rearranged_endpoints [num_endpoints];
 // rearrange_contours2 (endpoints, num_endpoints, rearranged_endpoints);
  unsigned int cutoff = rearrange_contours2 (endpoints, num_endpoints, rearranged_endpoints);
  endpoints = rearranged_endpoints;
  
 // printf("The cutoff is %d out of a total of %d.\n", cutoff, num_endpoints);
      
  for (int row = 0; row < grid_h; row++)
    for (int col = 0; col < grid_w; col++)
    {
      Point cp0 = origin + Vector ((col + 0) * cell_unit, (row + 0) * cell_unit);
      Point cp1 = origin + Vector ((col + 1) * cell_unit, (row + 1) * cell_unit);
      near_endpoints.clear ();

      int side;
      unsigned int num_group_1_arcs = 0;
      closest_arcs_to_cell (cp0, cp1,
			    faraway,
			    endpoints, num_endpoints, cutoff,
			    &num_group_1_arcs, near_endpoints,
			    &side);
  //    printf("Cell (%d,%d) has corner (%f,%f) and is close to %d arcs.\n", row, col, cp0.x, cp0.y, near_endpoints.size ());		    


#define QUANTIZE_X(X) (lround (MAX_X * ((X - extents.min_x) / glyph_width )))
#define QUANTIZE_Y(Y) (lround (MAX_Y * ((Y - extents.min_y) / glyph_height)))
#define DEQUANTIZE_X(X) (double (X) / MAX_X * glyph_width  + extents.min_x)
#define DEQUANTIZE_Y(Y) (double (Y) / MAX_Y * glyph_height + extents.min_y)
#define SNAP(P) (Point (DEQUANTIZE_X (QUANTIZE_X ((P).x)), DEQUANTIZE_Y (QUANTIZE_Y ((P).y))))

      if (near_endpoints.size () == 2 && near_endpoints[1].d == 0) {
        Point c (extents.min_x + glyph_width * .5, extents.min_y + glyph_height * .5);
        Line line (SNAP (near_endpoints[0].p), SNAP (near_endpoints[1].p));
	line.c -= line.n * Vector (c);
	line.c /= unit;
	tex_data[row * grid_w + col] = line_encode (line);
	continue;
      }

      /* If the arclist is two arcs that can be combined in encoding if reordered,
       * do that. */
      if (near_endpoints.size () == 4 &&
	  isinf (near_endpoints[2].d) &&
	  near_endpoints[0].p.x == near_endpoints[3].p.x &&
	  near_endpoints[0].p.y == near_endpoints[3].p.y)
      {
        glyphy_arc_endpoint_t e0, e1, e2;
	e0 = near_endpoints[2];
	e1 = near_endpoints[3];
	e2 = near_endpoints[1];
	near_endpoints.resize (0);
	near_endpoints.push_back (e0);
	near_endpoints.push_back (e1);
	near_endpoints.push_back (e2);
      }

      for (unsigned i = 0; i < near_endpoints.size (); i++) {
        glyphy_arc_endpoint_t &endpoint = near_endpoints[i];
	tex_data.push_back (arc_endpoint_encode (QUANTIZE_X(endpoint.p.x), QUANTIZE_Y(endpoint.p.y), endpoint.d));
      }

      unsigned int current_endpoints = tex_data.size () - offset;

      /* See if we can fulfill this cell by using already-encoded arcs */
      const glyphy_rgba_t *needle = &tex_data[offset];
      unsigned int needle_len = current_endpoints;
      const glyphy_rgba_t *haystack = &tex_data[header_length];
      unsigned int haystack_len = offset - header_length;

      bool found = false;

     if (needle_len)
	while (haystack_len >= needle_len) {
	  /* Trick: we don't care about first endpoint's d value, so skip one
	   * byte in comparison.  This works because arc_encode() packs the
	   * d value in the first byte. */
	  if (0 == memcmp (1 + (const char *) needle,
			   1 + (const char *) haystack,
			   needle_len * sizeof (*needle) - 1)) {
	    found = true;
	    break;
	  }
	  haystack++;
	  haystack_len--;
	}
      if (found) {
	tex_data.resize (offset);
	offset = haystack - &tex_data[0];
      }
        

      tex_data[row * grid_w + col] = arc_list_encode (num_group_1_arcs, offset, current_endpoints, side);
      offset = tex_data.size ();

      total_arcs += current_endpoints;
    }

  if (avg_fetch_achieved)
    *avg_fetch_achieved = 1 + double (total_arcs) / (grid_w * grid_h);

  *pextents = extents;

  if (tex_data.size () > blob_size)
    return false;

  memcpy (blob, &tex_data[0], tex_data.size () * sizeof(tex_data[0]));
  *output_len = tex_data.size ();
  *nominal_width = grid_w;
  *nominal_height = grid_h;

  return true;
}
