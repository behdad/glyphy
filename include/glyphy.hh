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
#include <glyphy/config.h>

namespace GLyphy {

  const char *const FSCODE_PATH = GLYPHY_SHADERDIR "/fragment_shader.glsl";

  int
  arcs_to_texture (std::vector<Geometry::arc_t> &arcs, int width, int *height,
		   void **buffer);

} /* namespace GLyphy */

#if 0
// Large font size profile
#define MIN_FONT_SIZE 64
#define TOLERANCE 5e-4
#define GRID_SIZE 16
#else
// Small font size profile
#define MIN_FONT_SIZE 20
#define TOLERANCE 5e-3
#define GRID_SIZE 16
#endif

#endif
