#ifndef GLYPHY_H
#define GLYPHY_H

#include <GL/glew.h>
#if defined(__APPLE__)
    #include <Glut/glut.h>
#else
    #include <GL/glut.h>
#endif

#include <vector>
#include "geometry.hh"
#include "freetype-helper.hh"

namespace GLyphy {

  typedef Geometry::Point<Geometry::Coord> point_t;
  typedef Geometry::Arc<Geometry::Coord, Geometry::Scalar> arc_t;

  struct rgba_t {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
  };

  void
  closest_arcs_to_cell (point_t p0, point_t p1, /* corners */
			double grid_size,
			const std::vector<arc_t> &arcs,
			std::vector<arc_t> &near_arcs,
			bool &inside_glyph);

  const struct rgba_t
  arc_encode (double x, double y, double d);

  struct rgba_t
  pair_to_rgba (unsigned int num1, unsigned int num2);

  int
  generate_texture (unsigned int upem, FT_Outline *outline, int width,
		    int *height, void **buffer);

  FT_Outline *
  face_to_outline (FT_Face face, unsigned int glyph_index);

} /* namespace GLyphy */

#endif
