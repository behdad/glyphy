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


using namespace std;
using namespace Geometry;

typedef Vector<Coord> vector_t;
typedef Point<Coord> point_t;
typedef Line<Coord> line_t;
typedef Segment<Coord> segment_t;
typedef Circle<Coord, Scalar> circle_t;
typedef Arc<Coord, Scalar> arc_t;
typedef Bezier<Coord> bezier_t;

struct rgba_t {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};

GLuint
compile_shader (GLenum type, const GLchar* source);

GLuint
link_program (GLuint vshader, GLuint fshader);

void
closest_arcs_to_cell (point_t p0, point_t p1, /* corners */
		      double grid_size,
		      const vector<arc_t> &arcs,
		      vector<arc_t> &near_arcs,
		      bool &inside_glyph);

const rgba_t
arc_encode (double x, double y, double d);

rgba_t
pair_to_rgba (unsigned int num1, unsigned int num2);

GLint
create_texture (const char *font_path, const char UTF8);

GLuint
create_program (void);

#endif
