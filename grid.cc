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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <cairo-ft.h>

#include "geometry.hh"
#include "cairo-helper.hh"
#include "freetype-helper.hh"
#include "sample-curves.hh"
#include "bezier-arc-approximation.hh"


using namespace std;
using namespace Geometry;
using namespace CairoHelper;
using namespace FreeTypeHelper;
using namespace SampleCurves;
using namespace BezierArcApproximation;

typedef Vector<Coord> vector_t;
typedef Point<Coord> point_t;
typedef Line<Coord> line_t;
typedef Segment<Coord> segment_t;
typedef Circle<Coord, Scalar> circle_t;
typedef Arc<Coord, Scalar> arc_t;
typedef Bezier<Coord> bezier_t;

/* These control the dimensions of the grid overlaying the gylph. */
static int grid_span_x = 8;
static int grid_span_y = 8;


/** Given a cell, fills the vector closest_arcs with arcs that may be closest to some point in the cell.
  * Uses idea that all close arcs to cell must be ~close to center of cell. 
  */
static void 
closest_arcs_to_cell (Point<Coord> square_top_left, 
                        Scalar cell_width, 
                        Scalar cell_height,
                        vector <Arc <Coord, Scalar> > arc_list,
                        vector <Arc <Coord, Scalar> > &closest_arcs)
{
  closest_arcs.clear ();

  /* Find distance between cell center and cell's closest arc. */
  Point<Coord> center (square_top_left.x + cell_width / 2., 
                       square_top_left.y + cell_height / 2.);
  double min_distance = INFINITY;
  Point<Coord> closest (0, 0);
  for (int k = 0; k < arc_list.size (); k++) 
    if (fabs(arc_list.at (k).distance_to_point (center)) < min_distance) {
      min_distance = fabs(arc_list.at (k).distance_to_point (center));
      closest = arc_list.at (k).nearest_part_to_point (center);
    }
    
  /* If d is the distance from the center of the square to the nearest arc, then
     all nearest arcs to the square must be at most [d + s/sqrt(2)] from the center. */
  Scalar radius = min_distance + sqrt (cell_height * cell_height + cell_width * cell_width) / 2 ;
  
  for (int k = 0; k < arc_list.size (); k++) {
    if (fabs(arc_list.at(k).distance_to_point (center)) < radius) {
      closest_arcs.push_back (arc_list.at (k));      
    }
      
  }
}

/** Given a cell, fills the vector closest_arcs with arcs that may be closest to some point in the cell.
  * Uses idea that all close arcs to cell must have at most min_max_min distance to cell. 
  */
static void 
closest_arcs_to_cell_v2 (Point<Coord> square_top_left, 
                          Scalar cell_width, 
                          Scalar cell_height,
                          vector <Arc <Coord, Scalar> > arc_list,
                          vector <Arc <Coord, Scalar> > &closest_arcs)
{
  closest_arcs.clear ();

  Quad<Coord> cell (square_top_left, cell_width, cell_height);
  double min_distance = INFINITY;

  Point<Coord> closest (0, 0);
  for (int k = 0; k < arc_list.size (); k++) 
    if (cell.max_distance_to_arc(arc_list.at (k)) < min_distance) {  /********************************************* If there is a problem in this function, it might be because max_distance_to needs fixing. ******/
      min_distance = cell.max_distance_to_arc(arc_list.at (k));
    }
    
  for (int k = 0; k < arc_list.size (); k++) 
    if (cell.distance_to_arc(arc_list.at (k)) <= min_distance) {
      closest_arcs.push_back (arc_list.at (k));  
    }
}


/** Given a grid, finds the number of arcs close to every cell in the grid.
  * Returns max_e and min_e with the highest and lowest number of arcs, respectively.
  */
static double
calc_closest_arcs (const std::vector<double> &grid_x_posns,
                   const std::vector<double> &grid_y_posns,
		   std::vector<int> &e,  std::vector<int> &e_y,
		   std::vector<Arc<Coord, Scalar> > &arc_list,
		   int &max_e, int &min_e)
{

  unsigned int n = grid_x_posns.size () - 1;
  unsigned int m = grid_y_posns.size () - 1;
  e.resize (n);
  e.resize (m);
  max_e = 0;
  min_e = (int) arc_list.size ();
  
  vector <Arc <Coord, Scalar> > closest_arcs;
  for (unsigned int i = 0; i < n; i++)  {
    e.at(i) = 0;
    for (int j = 0; j < m; j++) {
      closest_arcs_to_cell (Point<Coord> (grid_x_posns.at (i), grid_y_posns.at (j)), 
                         grid_x_posns.at (i + 1) - grid_x_posns.at (i),
                         grid_y_posns.at (j + 1) - grid_y_posns.at (j),                     
                         arc_list, closest_arcs);                 
      e.at(i) = std::max ( e.at(i), (int) closest_arcs.size ());
      e_y.at(j) = std::max ( e_y.at(j), (int) closest_arcs.size ());
    }
    max_e = std::max (max_e, e.at(i));
    min_e = std::min (min_e, e.at(i));
  }

}

/** Given a point, finds the shortest distance to the arc that is closest to that point. 
  * Sign of the distance depends on whether point is "inside" or "outside" the glyph.
  */
static double
distance_to_an_arc (Point<Coord> p,
                    vector <Arc <Coord, Scalar> > arc_list)
{
  double min_distance = INFINITY;
  int nearest_arc = 0;
  int k;
  for (k = 0; k < arc_list.size (); k++)  {
    double current_distance = arc_list.at(k).distance_to_point (p);    
    if (fabs (current_distance) == fabs(min_distance)) { /** Rounding does not seem to be a problem here. **/
      SignedVector<Coord> to_arc_min = arc_list.at(nearest_arc) - p;
      SignedVector<Coord> to_arc_current = arc_list.at(k) - p;
      
      if (to_arc_min.len () < to_arc_current.len ()) {
        min_distance = fabs (min_distance) * (to_arc_current.negative ? -1 : 1);
      }
    }
    else if (fabs (current_distance) < fabs(min_distance)) {
      min_distance = current_distance;
      nearest_arc = k;
    }
  }
  return min_distance;
}
                     
/** Draws line segments joining cell centres to the arcs at their closest points. 
  * Hugely inefficient, unnecessary EXCEPT for visual verification purposes. 
  *   Try **not** to use, if possible. Thanks!
  */
static void 
draw_lines_to_closest_arcs (cairo_t *cr, vector <Arc <Coord, Scalar> > arc_list) 
{
 
  Point<Coord> closest (0, 0);

  
 
 double box_width = 5;
  double box_height = 5;
  double grid_min_x = INFINITY; //-20;
  double grid_max_x = -1 * INFINITY; //350;
  double grid_min_y = INFINITY; //-120;
  double grid_max_y = -1 * INFINITY; //210;
  
  
 /* Autogenerate a  nice-filling grid max_width, max_height. */
  for (int i = 0; i < arc_list.size (); i++)  {
    Arc<Coord, Scalar> arc = arc_list.at(i);
    grid_min_x = std::min (grid_min_x, (fabs(arc.d) < 0.4 ? std::min (arc.p0.x, arc.p1.x) : arc.center().x - arc.radius()));
    grid_max_x = std::max (grid_max_x, (fabs(arc.d) < 0.4 ? std::max (arc.p0.x, arc.p1.x) : arc.center().x + arc.radius()));
    grid_min_y = std::min (grid_min_y, (fabs(arc.d) < 0.4 ? std::min (arc.p0.y, arc.p1.y) : arc.center().y - arc.radius()));
    grid_max_y = std::max (grid_max_y, (fabs(arc.d) < 0.4 ? std::max (arc.p0.y, arc.p1.y) : arc.center().y + arc.radius()));
  }
  
 
  box_width = (grid_max_x - grid_min_x) / grid_span_x;
  box_height = (grid_max_y - grid_min_y) / grid_span_y;
  grid_min_y -= 1 * box_height;
  
  /* Uncomment these to make the grid fit more loosely around the glyph. */
//  grid_min_x -= 1 * box_width; 
//  grid_max_x += 1 * box_width;
  grid_max_y -= 1 * box_height;    
//  grid_min_y -= 1 * box_height;
 
  int counter = 0;
  for (double j = grid_max_y; j > grid_min_y; j-= box_height) {
    for (double i = grid_min_x; i < grid_max_x; i+= box_width) {
    
      counter++;
      
      double min_distance = INFINITY;
      Point<Coord> center (i + box_width / 2., 
                       j + box_height / 2.);
      for (int k = 0; k < arc_list.size (); k++) 
        if (fabs(arc_list.at (k).distance_to_point (center)) < min_distance) {
          min_distance = fabs(arc_list.at (k).distance_to_point (center));
          closest = arc_list.at (k).nearest_part_to_point (center);
        }

        cairo_set_source_rgb (cr, 1, 0, 0);
        cairo_move_to (cr, center.x, center.y);
        cairo_line_to (cr, closest.x, closest.y);
        cairo_stroke (cr);
        
        cairo_set_line_width (cr, cairo_get_line_width (cr) * 2);
        cairo_set_source_rgb (cr, 0, 0, 1); 
        cairo_demo_point (cr, closest);
        cairo_stroke (cr);       
        cairo_set_line_width (cr, cairo_get_line_width (cr) / 4);
        
        if (counter == 35) {
        cairo_set_source_rgb (cr, 0.9, 0.9, 1);
        cairo_arc (cr, center.x, center.y, min_distance + sqrt (box_height * box_height + box_width * box_width) / 2 , 0, 2*M_PI);
        cairo_stroke (cr); 
        }
        cairo_set_line_width (cr, cairo_get_line_width (cr) * 2);
    }
  }
  
 
}

/** Sets up a uniform grid. Compare every arc with every cell.
  * Uses function closest_arcs_to_cell (v1): square root + distance_to_center. 
  */
static void 
gridify_and_find_arcs (cairo_t *cr, vector <Arc <Coord, Scalar> > arc_list, bool paint)
{

 
  double box_width = 5;
  double box_height = 5;
  double grid_min_x = INFINITY; //-20;
  double grid_max_x = -1 * INFINITY; //350;
  double grid_min_y = INFINITY; //-120;
  double grid_max_y = -1 * INFINITY; //210;
  
  
 /* Autogenerate a  nice-filling grid max_width, max_height. */
  for (int i = 0; i < arc_list.size (); i++)  {
    Arc<Coord, Scalar> arc = arc_list.at(i);
    grid_min_x = std::min (grid_min_x, (fabs(arc.d) < 0.4 ? std::min (arc.p0.x, arc.p1.x) : arc.center().x - arc.radius()));
    grid_max_x = std::max (grid_max_x, (fabs(arc.d) < 0.4 ? std::max (arc.p0.x, arc.p1.x) : arc.center().x + arc.radius()));
    grid_min_y = std::min (grid_min_y, (fabs(arc.d) < 0.4 ? std::min (arc.p0.y, arc.p1.y) : arc.center().y - arc.radius()));
    grid_max_y = std::max (grid_max_y, (fabs(arc.d) < 0.4 ? std::max (arc.p0.y, arc.p1.y) : arc.center().y + arc.radius()));
  }
  
 
  box_width = (grid_max_x - grid_min_x) / grid_span_x;
  box_height = (grid_max_y - grid_min_y) / grid_span_y;
  grid_min_y -= 1 * box_height;
  
  /* Uncomment these to make the grid fit more loosely around the glyph. */
//  grid_min_x -= 1 * box_width; 
//  grid_max_x += 1 * box_width;
  grid_max_y -= 1 * box_height;    
//  grid_min_y -= 1 * box_height;
  
//  printf("Grid: [%g, %g] x [%g, %g] with box width %g.\n", grid_min_x, grid_max_x, grid_min_y, grid_max_y, box_width); 
 
 
  
  int min_arcs = arc_list.size();
  int max_arcs = 0;
  int sum_arcs = 0;
  int num_arcs = 0;
  
  printf("Arcs per cell: ");
  
  for (double curr_y = grid_max_y; curr_y > grid_min_y; curr_y-= (box_height)) {
    for (double curr_x = grid_min_x; curr_x < grid_max_x; curr_x+= (box_width)) {
      
      vector <Arc <Coord, Scalar> > closest_arcs;
      closest_arcs_to_cell (Point<Coord> (curr_x, curr_y), 
                              box_width, box_height, arc_list, closest_arcs);


/* This handles the drawing of the grid. */
  if (paint) {
      double gradient = closest_arcs.size () * 1.2 / arc_list.size ();
      cairo_set_source_rgb (cr, 0.8 * gradient, 1.7 * gradient, 1.1 * gradient);
     
     cairo_move_to (cr, curr_x, curr_y);
     cairo_rel_line_to (cr, box_width, 0);
     cairo_rel_line_to (cr, 0, box_height);
     cairo_rel_line_to (cr, -1 * box_width, 0);
     cairo_close_path (cr);

     cairo_set_line_width (cr, 150);
     cairo_fill_preserve (cr);
     /* Easiest way to remove grid outlines is to comment this one liner out. */
    // cairo_set_source_rgb (cr, 0, 0, 0);
     
//     cairo_demo_point (cr, Point<Coord>(curr_x, curr_y));
     cairo_stroke (cr);
     
  }
      
     
     
     min_arcs = std::min (min_arcs, (int) closest_arcs.size ());
     max_arcs = std::max (max_arcs, (int) closest_arcs.size ());
     sum_arcs += closest_arcs.size ();
     num_arcs++;

     printf("%d ", (int) (closest_arcs.size ()));
    }
//    printf("\n");
  }
  
  printf ("; Min arcs per cell: %d; Max arcs per cell: %d; Mean arcs per cell: %g;.\n", 
         min_arcs, max_arcs, sum_arcs * 1.0 / num_arcs);
//  draw_lines_to_closest_arcs (cr, arc_list);
}



/** Given a grid, tolerance, and error values, jiggles the horizontal and vertical 
  * grid spacings to try and reduce the maximum number of arcs in any cell. 
  */
static inline void jiggle (  std::vector<double> &grid_posns_x,
			     std::vector<double> &grid_posns_y,
			     std::vector<int> &e,
			     std::vector<int> &e_y,
			     std::vector<Arc<Coord, Scalar> > &arc_list,
			     int &max_e, int &min_e,
			     double tolerance,
			     unsigned int &n_jiggle)
  {
    unsigned int n = grid_posns_x.size () - 1;
    unsigned int m = grid_posns_y.size () - 1;
    //fprintf (stderr, "candidate n %d max_e %g min_e %g\n", n, max_e, min_e);
    unsigned int max_jiggle = log2 (std::max(m,n));
    unsigned int s;
    
    double total_width = grid_posns_x.at(n) - grid_posns_x.at(0);
    double total_height = grid_posns_y.at(n) - grid_posns_y.at(0);
    
    for (s = 0; s <= max_jiggle; s++)
    {
      // Jiggle x direction.
      double total = 0;
      for (unsigned int i = 0; i < n; i++) {
	double l = grid_posns_x[i + 1] - grid_posns_x[i];
	double k_inv = l * pow (e[i], -1);
	total += k_inv;
	e[i] = k_inv;
      }
      for (unsigned int i = 0; i < n; i++) {
	double k_inv = e[i];
	double l = total_width * k_inv / total;
	grid_posns_x[i + 1] = grid_posns_x[i] + l;
      }
      
      // Jiggle y direction.
      total = 0;
      for (unsigned int i = 0; i < m; i++) {
	double l = grid_posns_y[i + 1] - grid_posns_y[i];
	double k_inv = l * pow (e_y[i], -1);
	total += k_inv;
	e_y[i] = k_inv;
      }
      for (unsigned int i = 0; i < m; i++) {
	double k_inv = e[i];
	double l = total_height * k_inv / total;
	grid_posns_y[i + 1] = grid_posns_y[i] + l;
      }
      
      

      for (unsigned int i = 0; i < n; i++)
        calc_closest_arcs (grid_posns_x, grid_posns_y, e, e_y, arc_list, max_e, min_e);

      //fprintf (stderr, "n %d jiggle %d max_e %g min_e %g\n", n, s, max_e, min_e);

      n_jiggle++;
      if (max_e < tolerance || (2 * min_e - max_e > tolerance))
	break;
    }
    //if (s == max_jiggle) fprintf (stderr, "JIGGLE OVERFLOW n %d s %d\n", n, s);
  }







/** Sets up a nonuniform grid by starting with uniform spacing and then jiggling in 
  * an attempt to optimize. Then, compare every arc with every cell.
  * Uses function closest_arcs_to_cell (v1): square root + distance_to_center. 
  **** Turns out this isn't so useful. Avoid in general. ***
  */
static void 
gridify_and_find_arcs_v2 (cairo_t *cr, vector <Arc <Coord, Scalar> > arc_list)
{

//  double tolerance = 3;

  double grid_min_x = INFINITY; //-20;
  double grid_max_x = -1 * INFINITY; //350;
  double grid_min_y = INFINITY; //-120;
  double grid_max_y = -1 * INFINITY; //210;
  
  
 
  for (int i = 0; i < arc_list.size (); i++)  {
    Arc<Coord, Scalar> arc = arc_list.at(i);
    grid_min_x = std::min (grid_min_x, (fabs(arc.d) < 0.5 ? std::min (arc.p0.x, arc.p1.x) : arc.center().x - arc.radius()));
    grid_max_x = std::max (grid_max_x, (fabs(arc.d) < 0.5 ? std::max (arc.p0.x, arc.p1.x) : arc.center().x + arc.radius()));
    grid_min_y = std::min (grid_min_y, (fabs(arc.d) < 0.5 ? std::min (arc.p0.y, arc.p1.y) : arc.center().y - arc.radius()));
    grid_max_y = std::max (grid_max_y, (fabs(arc.d) < 0.5 ? std::max (arc.p0.y, arc.p1.y) : arc.center().y + arc.radius()));
  }
  
  
  double box_width = (grid_max_x - grid_min_x) / grid_span_x;
  double box_height = (grid_max_y - grid_min_y) / grid_span_y;
  grid_min_y -= 2 * box_height;
  grid_max_y += 2 * box_height;
  grid_min_x -= 2 * box_width;
  grid_max_x += 2 * box_width;

  
  int min_arcs = arc_list.size();
  int max_arcs = 0;
  int sum_arcs = 0;
  int num_arcs = 0;
  
    
  printf("Grid: [%g, %g] x [%g, %g] with box width %g and box height %g.\n", grid_min_x, grid_max_x, grid_min_y, grid_max_y, box_width, box_height); 
 
    std::vector<double> grid_posns_x;
    std::vector<double> grid_posns_y;
    std::vector<int> e, e_y;
    

    int max_e, min_e;
    unsigned int n_jiggle = 0;
    double tolerance = std::max(5.00, 0.05 * arc_list.size ()); /********************************************************************************* CHANGE. ******************************************/


    
    /* Technically speaking we can bsearch for n. */
    for (unsigned int n = 1; n <= grid_span_x; n++)
    {
      grid_posns_y.clear();
      grid_posns_x.clear();
      
      double cell_width = (grid_max_x - grid_min_x) / n;
      double cell_height = cell_width;
      
      grid_posns_y.push_back(grid_min_y);    
      for (unsigned int i = 1; grid_posns_y.at (i - 1) <= grid_max_y; i++) {
        grid_posns_y.push_back(grid_posns_y.at (i - 1) + cell_height);      
      }
      grid_posns_x.resize (n + 1);
      
      
      grid_posns_x[0] = grid_min_x;
      for (unsigned int i = 1; i <= n; i++)
        grid_posns_x[i] = grid_posns_x[i - 1] + cell_width;

      calc_closest_arcs (grid_posns_x, grid_posns_y, e, e_y, arc_list, max_e, min_e);

      printf("n = %d. Min error is %d and max error is %d.\n", n, min_e, max_e);

      for (unsigned int i = 0; i < n; i++) {
        printf("e[%d] = %d.\t", i, e[i]);
	if (e[i] <= tolerance) {
	  jiggle (grid_posns_x, grid_posns_y, e, e_y, arc_list, max_e, min_e, tolerance, n_jiggle);
	  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~ Jiggled!");
	  break;
	}
      }
      
      printf("\nn = %d. NOW, min error is %d and max error is %d.\n\n", n, min_e, max_e);

      if (max_e <= tolerance)
        break;
    }
//    if (perror)
//      *perror = max_e;
    //fprintf (stderr, "n_jiggle %d\n", n_jiggle);
//    return arcs;
 
 
 /*************************************************************************/ 
  
  for (int i = 0; i < grid_posns_x.size() - 1; i++)
    printf("%g - (%g) - ", grid_posns_x.at(i), grid_posns_x.at(i+1) - grid_posns_x.at(i));
  printf("%g\n\n\n", grid_posns_x.at(grid_posns_x.size() -1 ));
  
  for (int i = 0; i < grid_posns_y.size() - 1; i++)
    printf("%g - (%g) - ", grid_posns_y.at(i), grid_posns_y.at(i+1) - grid_posns_y.at(i));
  printf("%g\n", grid_posns_y.at(grid_posns_y.size() -1 ));
  
  
  for (int j = 0; j < grid_posns_y.size() - 1; j++) {//j > 0; j--) {
   for (int i = 0; i < grid_posns_x.size () - 1; i++) {
    
      double curr_x = grid_posns_x.at (i);
      double curr_y = grid_posns_y.at (j);
      box_width = grid_posns_x.at (i + 1) - curr_x;
      box_height = grid_posns_y.at (j + 1) - curr_y;
      
      vector <Arc <Coord, Scalar> > closest_arcs;
      closest_arcs_to_cell (Point<Coord> (curr_x, curr_y), 
                              box_width, box_height, arc_list, closest_arcs);
      double gradient = closest_arcs.size () * 1.2 / arc_list.size ();
      cairo_set_source_rgb (cr, 0.8 * gradient, 1.7 * gradient, 1.1 * gradient);
     //  cairo_set_source_rgb (cr, 0, 0, 0);
     // cairo_set_line_width (cr, cairo_get_line_width (cr) * 30);
     // CairoHelper::cairo_demo_point (cr, Point<Coord> (i + box_width * 0.5 , j + box_width * 0.5));
     // cairo_set_line_width (cr, cairo_get_line_width (cr) / 30);
     
     cairo_move_to (cr, curr_x, curr_y);
     cairo_rel_line_to (cr, box_width, 0);
     cairo_rel_line_to (cr, 0, box_height);
     cairo_rel_line_to (cr, -1 * box_width, 0);
     cairo_close_path (cr);

     cairo_set_line_width (cr, std::max(box_width, box_height));
     cairo_fill_preserve (cr);
     cairo_set_source_rgb (cr, 0, 0, 0);
     cairo_stroke (cr);
     
     min_arcs = std::min (min_arcs, (int) closest_arcs.size ());
     max_arcs = std::max (max_arcs, (int) closest_arcs.size ());
     sum_arcs += closest_arcs.size ();

//     printf("(%g,%g): %d\t", i, j, (int) (closest_arcs.size ()));
    }
//    printf("\n");
  }
  
  printf ("Number of close arcs range from %d to %d, with average of %g.\n", 
         min_arcs, max_arcs, sum_arcs * 1.0 / (grid_posns_x.size() * grid_posns_y.size() ));
       
}






/** Uniform grid. Do not compare every arc with every cell using center of cell. 
  * Instead, use min_max_min distance approach. 
  * This actually is not too great. 
  */
static void 
gridify_and_find_arcs_v3 (cairo_t *cr, vector <Arc <Coord, Scalar> > arc_list)
{

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_set_line_width (cr, cairo_get_line_width (cr) * 32);
  double box_width = 5;
  double box_height = 5;
  double grid_min_x = INFINITY; //-20;
  double grid_max_x = -1 * INFINITY; //350;
  double grid_min_y = INFINITY; //-120;
  double grid_max_y = -1 * INFINITY; //210;
  
  
 
  for (int i = 0; i < arc_list.size (); i++)  {
    Arc<Coord, Scalar> arc = arc_list.at(i);
    grid_min_x = std::min (grid_min_x, (fabs(arc.d) < 0.5 ? std::min (arc.p0.x, arc.p1.x) : arc.center().x - arc.radius()));
    grid_max_x = std::max (grid_max_x, (fabs(arc.d) < 0.5 ? std::max (arc.p0.x, arc.p1.x) : arc.center().x + arc.radius()));
    grid_min_y = std::min (grid_min_y, (fabs(arc.d) < 0.5 ? std::min (arc.p0.y, arc.p1.y) : arc.center().y - arc.radius()));
    grid_max_y = std::max (grid_max_y, (fabs(arc.d) < 0.5 ? std::max (arc.p0.y, arc.p1.y) : arc.center().y + arc.radius()));
  }
  
 
  box_width = (grid_max_x - grid_min_x) / grid_span_x;
  box_height = (grid_max_y - grid_min_y) / grid_span_y;
  grid_min_x -= 1 * box_width;
  grid_min_y -= 2 * box_height;
  grid_max_x += 1 * box_width;
  grid_max_y += 1 * box_height;  


  
  printf("Grid: [%g, %g] x [%g, %g] with box width %g.\n", grid_min_x, grid_max_x, grid_min_y, grid_max_y, box_width); 
 
  
  int min_arcs = arc_list.size();
  int max_arcs = 0;
  int sum_arcs = 0;
  int num_arcs = 0;

 
  for (double curr_y = grid_max_y; curr_y > grid_min_y; curr_y-= box_height) {
    for (double curr_x = grid_min_x; curr_x < grid_max_x; curr_x+= box_width) {

     
      vector <Arc <Coord, Scalar> > closest_arcs;
      closest_arcs_to_cell_v2 (Point<Coord> (curr_x, curr_y), 
                              box_width, box_height, arc_list, closest_arcs);  /********************************************* If there is a problem, it's probably in closest_arcs_to_cell_v2. ***************/
      double gradient = closest_arcs.size () * 1.2 / arc_list.size ();
      cairo_set_source_rgb (cr, 0.8 * gradient, 1.7 * gradient, 1.1 * gradient);
     //  cairo_set_source_rgb (cr, 0, 0, 0);
     // cairo_set_line_width (cr, cairo_get_line_width (cr) * 30);
     // CairoHelper::cairo_demo_point (cr, Point<Coord> (i + box_width * 0.5 , j + box_width * 0.5));
     // cairo_set_line_width (cr, cairo_get_line_width (cr) / 30);
     
     cairo_move_to (cr, curr_x, curr_y);
     cairo_rel_line_to (cr, box_width, 0);
     cairo_rel_line_to (cr, 0, box_height);
     cairo_rel_line_to (cr, -1 * box_width, 0);
     cairo_close_path (cr);

     cairo_set_line_width (cr, 150.0);
     cairo_fill_preserve (cr);
     cairo_set_source_rgb (cr, 0, 0, 0);
     cairo_stroke (cr);
     
     min_arcs = std::min (min_arcs, (int) closest_arcs.size ());
     max_arcs = std::max (max_arcs, (int) closest_arcs.size ());
     sum_arcs += closest_arcs.size ();
     num_arcs++;

     printf("(%g,%g): %d\t", curr_x, curr_y, (int) (closest_arcs.size ())); 
     
    }
//    printf("\n");
  }
  
  printf ("Number of close arcs range from %d to %d, with average of %g.\n", 
         min_arcs, max_arcs, sum_arcs * 1.0 / num_arcs);
//  draw_lines_to_closest_arcs (cr, arc_list);
}



static void
draw_distance_field (cairo_t *cr, vector <Arc <Coord, Scalar> > arc_list)
{

/*  double box_width = 5;
  double grid_min_x = INFINITY; //-20;
  double grid_max_x = -1 * INFINITY; //350;
  double grid_min_y = INFINITY; //-120;
  double grid_max_y = -1 * INFINITY; //210;
  
  double grid_min_x = -16000;//1000; //15;
  double grid_max_x = 140000; //33000; //25;
  double grid_min_y = -45000; //-15000; //-3;
  double grid_max_y = 120000; //45000; //30000; //5;
  double box_width = 200; //0.01;   */
  
  
  double grid_min_x = INFINITY; //-20;
  double grid_max_x = -1 * INFINITY; //350;
  double grid_min_y = INFINITY; //-120;
  double grid_max_y = -1 * INFINITY; //210;

  

  for (int i = 0; i < arc_list.size (); i++)  {
    Arc<Coord, Scalar> arc = arc_list.at(i);
    grid_min_x = std::min (grid_min_x, (fabs(arc.d) < 0.5 ? std::min (arc.p0.x, arc.p1.x) : arc.center().x - arc.radius()));
    grid_max_x = std::max (grid_max_x, (fabs(arc.d) < 0.5 ? std::max (arc.p0.x, arc.p1.x) : arc.center().x + arc.radius()));
    grid_min_y = std::min (grid_min_y, (fabs(arc.d) < 0.5 ? std::min (arc.p0.y, arc.p1.y) : arc.center().y - arc.radius()));
    grid_max_y = std::max (grid_max_y, (fabs(arc.d) < 0.5 ? std::max (arc.p0.y, arc.p1.y) : arc.center().y + arc.radius()));
  }
  
  
  
  double box_width = (grid_max_x - grid_min_x) / 100;
  double box_height = (grid_max_y - grid_min_y) / 100;
  

  grid_min_x -= 10 * 2 * box_width;
  grid_min_y -= 20 * 2 * box_height;
  grid_max_x += 10 * 2 * box_width;
  grid_max_y += 10 * 2 * box_height;  
  
  cairo_set_line_width (cr, std::min(box_width, box_height)); // cairo_get_line_width (cr) * 15);
  for (double i = grid_min_x; i < grid_max_x; i+= box_width)
  {
    for (double j = grid_min_y; j < grid_max_y; j+= box_height) {
      double gradient = distance_to_an_arc (Point<Coord> (i, j), arc_list) / (box_width * 30);
      if ((gradient *= 1) >= 0)
        cairo_set_source_rgb (cr, gradient * 0.6, gradient * 1.3, gradient * 1.0);
      else
         cairo_set_source_rgb (cr, gradient * (-1.3), gradient * (-0.6), gradient * (-1.0));  /********** Magenta **********/
  //      cairo_set_source_rgb (cr, gradient * (-1.7), gradient * (-1.7), gradient * (-0.5));    /********** Yellow **********/
      CairoHelper::cairo_demo_point (cr, Point<Coord> (i + box_width * 0.5 , j + box_width * 0.5));
    }
  }
  
  cairo_set_line_width (cr, cairo_get_line_width (cr) / 15);
}

static void
demo_curve (cairo_t *cr, const bezier_t &b)
{
 cairo_save (cr);
  cairo_set_line_width (cr, 5);

  cairo_curve (cr, b);
  cairo_set_viewport (cr);
  cairo_new_path (cr);

  cairo_set_source_rgb (cr, 1, 0, 0);
  cairo_demo_curve (cr, b);

  typedef MaxDeviationApproximatorExact MaxDev;
  typedef BezierArcErrorApproximatorBehdad<MaxDev> BezierArcError;
  typedef BezierArcApproximatorMidpointTwoPart<BezierArcError> BezierArcApproximator;
  typedef BezierArcsApproximatorSpringSystem<BezierArcApproximator> SpringSystem;

  double tolerance = 100.01;
  double e;
  std::vector<Arc<Coord, Scalar> > &arcs = SpringSystem::approximate_bezier_with_arcs (b, tolerance, &e);

  printf ("Num arcs %d; Approximation error %g; Tolerance %g; Percentage %g; %s\n",
	  (int) arcs.size (), e, tolerance, round (100 * e / tolerance), e <= tolerance ? "PASS" : "FAIL");

 // gridify_and_find_arcs (cr, arcs);
  draw_distance_field (cr, arcs);
  cairo_set_source_rgb (cr, 0.9, 0.9, 0.9);
  cairo_demo_curve (cr, b);

  cairo_set_source_rgba (cr, 1.0, 0.2, 0.2, 1.0);
  cairo_set_line_width (cr, cairo_get_line_width (cr) / 2);
  for (unsigned int i = 0; i < arcs.size (); i++)
  {
    cairo_demo_arc (cr, arcs[i]);
    Arc<Coord, Scalar> other_arc (arcs[i].p0, arcs[i].p1, (1.0 - arcs[i].d) / (1.0 + arcs[i].d)); 
  //  cairo_demo_arc (cr, other_arc);
  }

 

  delete &arcs;

  cairo_restore (cr);
}





static void
demo_text_2 (cairo_t *cr, const char *family, const char *utf8)
{
  typedef MaxDeviationApproximatorExact MaxDev;
  typedef BezierArcErrorApproximatorBehdad<MaxDev> BezierArcError;
  typedef BezierArcApproximatorMidpointTwoPart<BezierArcError> BezierArcApproximator;
  typedef BezierArcsApproximatorSpringSystem<BezierArcApproximator> SpringSystem;
  typedef ArcApproximatorOutlineSink<SpringSystem> ArcApproximatorOutlineSink;

  double e;

  class ArcAccumulator
  {
    public:

    static bool callback (const arc_t &arc, void *closure)
    {
       ArcAccumulator *acc = static_cast<ArcAccumulator *> (closure);
       acc->arcs.push_back (arc);
       return true;
    }

    std::vector<arc_t> arcs;
  } acc;
  
  cairo_save (cr);
  cairo_scale (cr, 1, -1);
  cairo_select_font_face (cr, family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  
  FT_Face face = cairo_ft_scaled_font_lock_face (cairo_get_scaled_font (cr));
  unsigned int upem = face->units_per_EM;
  FT_Set_Char_Size (face, upem*64, upem*64, 0, 0);
  //double tolerance = upem * 64. / 256; //upem * 64. / 256;
  double tolerance = upem * 1e-3; /* in font design units */
  
  
 /*  double x = 0, y = 0;
  double dx = 1, dy = 1;
  cairo_user_to_device (cr, &x, &y);
  cairo_user_to_device_distance (cr, &dx, &dy);
  cairo_identity_matrix (cr);
  cairo_translate (cr, x, y);
  cairo_scale (cr, FONT_SIZE*dx/(upem*64), -FONT_SIZE*dy/(upem*64)); */
  
  
  /*********************************************************************************************************************************************************/
  
  printf("Font: %s, %s, %s.\n", FT_Get_Postscript_Name (face), face->family_name, face->style_name);
  
  
  for  (int i = 0; i < face->num_glyphs - 1; i++) {
  acc.arcs.clear ();
  
  if (FT_Load_Glyph (face,
		     i,
		     FT_LOAD_NO_BITMAP |
		     FT_LOAD_NO_HINTING |
		     FT_LOAD_NO_AUTOHINT |
		     FT_LOAD_NO_SCALE |
		     FT_LOAD_LINEAR_DESIGN |
		     FT_LOAD_IGNORE_TRANSFORM))
    abort ();
  assert (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE);
  //ArcApproximatorOutlineSink::approximate_glyph (&face->glyph->outline, acc.callback, static_cast<void *> (&acc), tolerance, &e);
  ArcApproximatorOutlineSink outline_arc_approximator (acc.callback,
						       static_cast<void *> (&acc),
						       tolerance);
  FreeTypeOutlineSource<ArcApproximatorOutlineSink>::decompose_outline (&face->glyph->outline,
									outline_arc_approximator);
  //cairo_ft_scaled_font_unlock_face (cairo_get_scaled_font (cr));

  char* glyph_name;
  FT_Get_Glyph_Name(face, i, glyph_name, 128);
  
  printf ("Glyph #%d: %s; Grid: %d by %d; Tolerance: %g; Num_arcs: %d; ", 
         i, glyph_name, grid_span_x, grid_span_y, tolerance, (int) acc.arcs.size ());
  gridify_and_find_arcs (cr, acc.arcs, false);

  
  
  
  
  

  }
  /*********************************************************************************************************************************************************/

  cairo_set_source_rgba (cr, 0.5, 0.9, 0.8, 1);// 0.9, 1.0, 0.9, .3);
  cairo_set_line_width (cr, 4*64);
  for (unsigned int i = 0; i < acc.arcs.size (); i++)
    cairo_demo_arc (cr, acc.arcs[i]);


  cairo_restore (cr);
}




static void
demo_text (cairo_t *cr, const char *family, const char *utf8)
{
  cairo_save (cr);
  cairo_select_font_face (cr, family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_line_width (cr, 5);
#define FONT_SIZE 2048
  cairo_set_font_size (cr, FONT_SIZE);

  cairo_text_path (cr, utf8);
  cairo_path_t *path = cairo_copy_path (cr);
  cairo_path_print_stats (path);
  cairo_path_destroy (path);
  cairo_set_viewport (cr);
  
  cairo_set_line_width (cr, 25);

  cairo_new_path (cr);
  cairo_set_source_rgb (cr, 0.02, 0.4, 0.05);
  cairo_text_path (cr, utf8);
  cairo_stroke (cr); //cairo_fill
  cairo_set_line_width (cr, 5);

  typedef MaxDeviationApproximatorExact MaxDev;
  typedef BezierArcErrorApproximatorBehdad<MaxDev> BezierArcError;
  typedef BezierArcApproximatorMidpointTwoPart<BezierArcError> BezierArcApproximator;
  typedef BezierArcsApproximatorSpringSystem<BezierArcApproximator> SpringSystem;
  typedef ArcApproximatorOutlineSink<SpringSystem> ArcApproximatorOutlineSink;

  double e;

  class ArcAccumulator
  {
    public:

    static bool callback (const arc_t &arc, void *closure)
    {
       ArcAccumulator *acc = static_cast<ArcAccumulator *> (closure);
       acc->arcs.push_back (arc);
       return true;
    }

    std::vector<arc_t> arcs;
  } acc;

  FT_Face face = cairo_ft_scaled_font_lock_face (cairo_get_scaled_font (cr));
  unsigned int upem = face->units_per_EM;
  FT_Set_Char_Size (face, upem*64, upem*64, 0, 0);
  //double tolerance = upem * 64. / 256; //upem * 64. / 256;
  double tolerance = upem * 1e-3; /* in font design units */
  
  
  if (FT_Load_Glyph (face, FT_Get_Char_Index (face, (FT_ULong) *utf8), FT_LOAD_NO_BITMAP))
    abort ();
  assert (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE);
  //ArcApproximatorOutlineSink::approximate_glyph (&face->glyph->outline, acc.callback, static_cast<void *> (&acc), tolerance, &e);
  ArcApproximatorOutlineSink outline_arc_approximator (acc.callback,
						       static_cast<void *> (&acc),
						       tolerance);
  FreeTypeOutlineSource<ArcApproximatorOutlineSink>::decompose_outline (&face->glyph->outline,
									outline_arc_approximator);
  cairo_ft_scaled_font_unlock_face (cairo_get_scaled_font (cr));


  printf ("Num arcs %d; Approximation error %g; Tolerance %g; Percentage %g; %s\n",
	  (int) acc.arcs.size (), e, tolerance, round (100 * e / tolerance), e <= tolerance ? "PASS" : "FAIL");

  double x = 0, y = 0;
  double dx = 1, dy = 1;
  cairo_user_to_device (cr, &x, &y);
  cairo_user_to_device_distance (cr, &dx, &dy);
  cairo_identity_matrix (cr);
  cairo_translate (cr, x, y);
  cairo_scale (cr, FONT_SIZE*dx/(upem*64), -FONT_SIZE*dy/(upem*64));

  cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, .5);
  point_t start (0, 0);
  for (unsigned int i = 0; i < acc.arcs.size (); i++) {
    if (!cairo_has_current_point (cr))
      start = acc.arcs[i].p0;
    cairo_arc (cr, acc.arcs[i]);
    if (acc.arcs[i].p1 == start) {
      cairo_close_path (cr);
      cairo_new_sub_path (cr);
    }
  }
//  cairo_fill (cr);
  
//  draw_distance_field (cr, acc.arcs);
  gridify_and_find_arcs (cr, acc.arcs, true);
  
  cairo_set_source_rgba (cr, 0.5, 0.9, 0.8, 1);// 0.9, 1.0, 0.9, .3);
  cairo_set_line_width (cr, 4*64); //4 * 
  for (unsigned int i = 0; i < acc.arcs.size (); i++)
    cairo_demo_arc (cr, acc.arcs[i]);


  cairo_restore (cr);
}




int main (int argc, char **argv)
{
  cairo_t *cr;
  char *filename;
  cairo_status_t status;
  cairo_surface_t *surface;

  if (argc != 2) {
    fprintf (stderr, "Usage: arc OUTPUT_FILENAME\n");
    return 1;
  }

  filename = argv[1];

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					1400, 800);
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);

 // demo_curve (cr, sample_curve_skewed ());
 // demo_curve (cr, sample_curve_riskus_simple ());   
 // demo_curve (cr, sample_curve_riskus_complicated ());
 // demo_curve (cr, sample_curve_s ());
 // demo_curve (cr, sample_curve_serpentine_c_symmetric ());
 // demo_curve (cr, sample_curve_serpentine_s_symmetric ());  //x
 // demo_curve (cr, sample_curve_serpentine_quadratic ());
 // demo_curve (cr, sample_curve_loop_cusp_symmetric ());  
 // demo_curve (cr, sample_curve_loop_gamma_symmetric ());
 // demo_curve (cr, sample_curve_loop_gamma_small_symmetric ());
 // demo_curve (cr, sample_curve_loop_o_symmetric ());  // x
 // demo_curve (cr, sample_curve_semicircle_top ());
 // demo_curve (cr, sample_curve_semicircle_bottom ());
 // demo_curve (cr, sample_curve_semicircle_left ());
 // demo_curve (cr, sample_curve_semicircle_right ());
 
 

 
  demo_text_2 (cr, "Verdana", "a");
  
 /* cairo_set_source_rgb (cr, 1, 0, 0);
  cairo_set_line_width (cr, 100);
  cairo_demo_point (cr, Point<Coord> (15000, 0)); 
  */
  cairo_destroy (cr);

  status = cairo_surface_write_to_png (surface, filename);
  cairo_surface_destroy (surface);

  if (status != CAIRO_STATUS_SUCCESS) {
    fprintf (stderr, "Could not save png to '%s'\n", filename);
    return 1;
  }

  return 0;
}
