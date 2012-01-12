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
#include "bezier-arc-approximation.hh"

namespace GLyphy {

  int
  arcs_to_texture (std::vector<Geometry::arc_t> &arcs, int width, int *height,
		   void **buffer);

} /* namespace GLyphy */

#endif
