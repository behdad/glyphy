#ifndef GLYPHY_H
#define GLYPHY_H

#include <GL/glew.h>
#if defined(__APPLE__)
    #include <Glut/glut.h>
#else
    #include <GL/glut.h>
#endif

#include <vector>
#include <glyphy/geometry.hh>
#include <glyphy/config.hh>

namespace GLyphy {

  const char *const FSCODE_PATH = GLYPHY_SHADERDIR "/fragment_shader.glsl";

  int
  arcs_to_texture (std::vector<Geometry::arc_t> &arcs, int width, int *height,
		   void **buffer);

} /* namespace GLyphy */

#endif
