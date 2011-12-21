#include <GL/glew.h>
#if defined(__APPLE__)
    #include <Glut/glut.h>
#else
    #include <GL/glut.h>
#endif

#include <glib.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <assert.h>
#include <string>
#include <list>

#include "geometry.hh"
#include "freetype-helper.hh"
#include "sample-curves.hh"
#include "bezier-arc-approximation.hh"

using namespace std;
using namespace Geometry;
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


static void
die (const char *msg)
{
  fprintf (stderr, "%s\n", msg);
  exit (1);
}

static GLuint
compile_shader (GLenum type, const GLchar* source)
{
  GLuint shader;
  GLint compiled;

  if (!(shader = glCreateShader(type)))
    return shader;

  glShaderSource (shader, 1, &source, 0);
  glCompileShader (shader);

  glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint info_len = 0;
    fprintf (stderr, "Shader failed to compile\n");
    glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 0) {
      char *info_log = (char*) malloc (info_len);
      glGetShaderInfoLog (shader, info_len, NULL, info_log);

      fprintf (stderr, "%s\n", info_log);
      free (info_log);
    }

    abort ();
  }

  return shader;
}
#define COMPILE_SHADER1(Type,Src) compile_shader (Type, "#version 120\n" #Src)
#define COMPILE_SHADER(Type,Src) COMPILE_SHADER1(Type,Src)
#define gl(name) \
	for (GLint __ee, __ii = 0; \
	     __ii < 1; \
	     (__ii++, \
	      (__ee = glGetError()) && \
	      (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, "gl" G_STRINGIFY (name) " failed with error %04X on line %d", __ee, __LINE__), 0))) \
	  gl##name

static GLuint
link_program (GLuint vshader, GLuint fshader)
{
  GLuint program;
  GLint linked;

  program = glCreateProgram();
  glAttachShader(program, vshader);
  glAttachShader(program, fshader);
  glLinkProgram(program);

  glGetProgramiv (program, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint info_len = 0;
    fprintf (stderr, "Program failed to link\n");
    glGetProgramiv (program, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 0) {
      char *info_log = (char*) malloc (info_len);
      glGetProgramInfoLog (program, info_len, NULL, info_log);

      fprintf (stderr, "%s\n", info_log);
      free (info_log);
    }

    abort ();
  }

  return program;
}






#define MIN_FONT_SIZE 20
#define GRID_SIZE 64
#define GRID_X GRID_SIZE
#define GRID_Y GRID_SIZE
#define TOLERANCE 1e-6







/* Given a cell, fills the vector closest_arcs with arcs that may be closest to some point in the cell.
 * Uses idea that all close arcs to cell must be ~close to center of cell.
 */
static void
closest_arcs_to_cell (point_t p0, point_t p1, /* corners */
		      double grid_size,
		      const vector<arc_t> &arcs,
		      vector<arc_t> &near_arcs,
		      bool &inside_glyph)
{
  inside_glyph = false;
  arc_t current_arc = arcs[0];

  // Find distance between cell center and its closest arc.
  point_t c = p0 + p1;
  
  SignedVector<Coord> to_arc_min = current_arc - c;
  double min_distance = INFINITY;
  
  for (int k = 0; k < arcs.size (); k++) {
    current_arc = arcs[k];
    
    // We can't use squared distance, because sign is important.
    double current_distance = current_arc.distance_to_point (c);    

    // If two arcs are equally close to this point, take the sign from the one whose extension is farther away. 
    // (Extend arcs using tangent lines from endpoints; this is done using the SignedVector operation "-".) 
    if (fabs (fabs (current_distance) - fabs(min_distance)) < 1e-6) { 
      SignedVector<Coord> to_arc_current = current_arc - c;      
      if (to_arc_min.len () < to_arc_current.len ()) {
        min_distance = fabs (current_distance) * (to_arc_current.negative ? -1 : 1);
      }
    }
    else
      if (fabs (current_distance) < fabs(min_distance)) {
      min_distance = current_distance;
      to_arc_min = current_arc - c;
    }
  }
  
  inside_glyph = (min_distance > 0); 
    
  // If d is the distance from the center of the square to the nearest arc, then
  // all nearest arcs to the square must be at most [d + s/sqrt(2)] from the center. 
  min_distance =  fabs (min_distance);
  
  // If d is the distance from the center of the square to the nearest arc, then
  // all nearest arcs to the square must be at most [d + half_diagonal] from the center.
  double half_diagonal = (c - p0).len ();
  double faraway = double (grid_size) / MIN_FONT_SIZE;
  double radius_squared = pow (min_distance + half_diagonal + faraway, 2);
  if (min_distance - half_diagonal <= faraway)
    for (int i = 0; i < arcs.size (); i++) {
      if (arcs[i].squared_distance_to_point (c) <= radius_squared)
        near_arcs.push_back (arcs[i]);
    }
}



/* Bit packing */

#define UPPER_BITS(v,bits,total_bits) ((v) >> ((total_bits) - (bits)))
#define LOWER_BITS(v,bits,total_bits) ((v) & ((1 << (bits)) - 1))
#define MIDDLE_BITS(v,bits,upper_bound,total_bits) (UPPER_BITS (LOWER_BITS (v, upper_bound, total_bits), bits, upper_bound))


struct rgba_t {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};



static const rgba_t
arc_encode (double x, double y, double d)
{
  rgba_t v;

  // lets do 10 bits for d, and 11 for x and y each 
  unsigned int ix, iy, id;
  ix = lround (x * 4095);
  g_assert (ix < 4096);
  iy = lround (y * 4095);
  g_assert (iy < 4096);
#define MAX_D .54 // TODO (0.25?)
  if (isinf (d))
    id = 0;
  else {
    g_assert (fabs (d) < MAX_D);

    id = lround (d * 127. / MAX_D + 128);

  }
  g_assert (id < 256);

  v.r = LOWER_BITS (ix, 8, 12);
  v.g = LOWER_BITS (iy, 8, 12);
  v.b = id;
  v.a = ((ix >> 8) << 4) | (iy >> 8);
  return v;
}


static rgba_t
pair_to_rgba (unsigned int num1, unsigned int num2)
{
  rgba_t v;
  v.r = UPPER_BITS (num1, 8, 24);
  v.g = MIDDLE_BITS (num1, 8, 16, 24);
  v.b = LOWER_BITS (num1, 8, 24);
  v.a = LOWER_BITS (num2, 8, 8);
  return v;
}



static GLint
create_texture (const char *font_path, const char UTF8)
{
  FT_Face face;
  FT_Library library;
  FT_Init_FreeType (&library);   
  FT_New_Face ( library, font_path, 0, &face );
  unsigned int upem = face->units_per_EM;
  double tolerance = upem * TOLERANCE; // in font design units


  // Arc approximation code.
  typedef MaxDeviationApproximatorExact MaxDev;
  typedef BezierArcErrorApproximatorBehdad<MaxDev> BezierArcError;
  typedef BezierArcApproximatorMidpointTwoPart<BezierArcError> BezierArcApproximator;
  typedef BezierArcsApproximatorSpringSystem<BezierArcApproximator> SpringSystem;
  typedef ArcApproximatorOutlineSink<SpringSystem> ArcApproximatorOutlineSink;
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

  if (FT_Load_Glyph (face,
		     FT_Get_Char_Index (face, (FT_ULong) UTF8),
		     FT_LOAD_NO_BITMAP |
		     FT_LOAD_NO_HINTING |
		     FT_LOAD_NO_AUTOHINT |
		     FT_LOAD_NO_SCALE |
		     FT_LOAD_LINEAR_DESIGN |
		     FT_LOAD_IGNORE_TRANSFORM))
    abort ();

  assert (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE);
  ArcApproximatorOutlineSink outline_arc_approximator (acc.callback,
						       static_cast<void *> (&acc),
						       tolerance);
  // The actual arc decomposition is done here.
  FreeTypeOutlineSource<ArcApproximatorOutlineSink>::decompose_outline (&face->glyph->outline,
  									outline_arc_approximator);
  double e = outline_arc_approximator.error;
  printf ("Num arcs %d; Approximation error %g; Tolerance %g; Percentage %g. %s\n",
	  (int) acc.arcs.size (), e, tolerance, round (100 * e / tolerance), e <= tolerance ? "PASS" : "FAIL");

  /* TODO: replace this with analytical extents of the arcs. */
  int grid_min_x = INFINITY;
  int grid_max_x = -INFINITY;
  int grid_min_y = INFINITY;
  int grid_max_y = -INFINITY;
  int glyph_width, glyph_height;
  
  for (int i = 0; i < acc.arcs.size (); i++) {
    grid_min_x = std::min(grid_min_x, (int) acc.arcs[i].leftmost ().x.floor ());
    grid_max_x = std::max(grid_max_x, (int) acc.arcs[i].rightmost ().x.ceil ());
    grid_min_y = std::min(grid_min_y, (int) acc.arcs[i].lowest ().y.floor ());
    grid_max_y = std::max(grid_max_y, (int) acc.arcs[i].highest ().y.ceil ());
  }
  
//  grid_min_x = face->glyph->metrics.horiBearingX;
//  grid_min_y = face->glyph->metrics.horiBearingY - face->glyph->metrics.height;
//  grid_max_x = face->glyph->metrics.horiBearingX + face->glyph->metrics.width;
//  grid_max_y = face->glyph->metrics.horiBearingY;

  glyph_width = grid_max_x - grid_min_x;
  glyph_height = grid_max_y - grid_min_y;

  /* XXX */
  glyph_width = glyph_height = std::max (glyph_width, glyph_height);



  // Make a 2d grid for arc/cell information.
  vector<rgba_t> tex_data;

  // near_arcs: Vector of arcs near points in this single grid cell
  vector<arc_t> near_arcs;

  double min_dimension = std::min(glyph_width, glyph_height);
  unsigned int header_length = GRID_X * GRID_Y;
  unsigned int offset = header_length;
  tex_data.resize (header_length);
  point_t origin = point_t (grid_min_x, grid_min_y);

  for (int row = 0; row < GRID_Y; row++)
    for (int col = 0; col < GRID_X; col++)
    {
      point_t cp0 = origin + vector_t ((col + 0.) * glyph_width / GRID_X, (row + 0.) * glyph_height / GRID_Y);
      point_t cp1 = origin + vector_t ((col + 1.) * glyph_width / GRID_X, (row + 1.) * glyph_height / GRID_Y);
      near_arcs.clear ();

      bool inside_glyph;
      closest_arcs_to_cell (cp0, cp1, min_dimension, acc.arcs, near_arcs, inside_glyph); 

#define ARC_ENCODE(p, d) \
	arc_encode (((p).x - grid_min_x) / glyph_width, \
		    ((p).y - grid_min_y) / glyph_height, \
		    (d))


      point_t p1 = point_t (0, 0);
      for (unsigned i = 0; i < near_arcs.size (); i++)
      {
        arc_t arc = near_arcs[i];

	if (p1 != arc.p0)
	  tex_data.push_back (ARC_ENCODE (arc.p0, INFINITY));

	tex_data.push_back (ARC_ENCODE (arc.p1, arc.d));
	p1 = arc.p1;
      }

      // Use the last bit to store whether or not the pixel is inside the glyph. 
      tex_data[row * GRID_X + col] = pair_to_rgba (offset, 2 * (tex_data.size () - offset) + (inside_glyph ? 1 : 0));
      offset = tex_data.size ();
    }

  unsigned int tex_len = tex_data.size ();
  unsigned int tex_w = 128;
  unsigned int tex_h = (tex_len + tex_w - 1) / tex_w;
  tex_data.resize (tex_w * tex_h);

  printf ("Texture size %ux%u; %'lu bytes\n", tex_w, tex_h, tex_w * tex_h * sizeof (tex_data[0]));

  GLuint texture;
  glGenTextures (1, &texture);
  glBindTexture (GL_TEXTURE_2D, texture);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  /* Upload*/
  gl(TexImage2D) (GL_TEXTURE_2D, 0, GL_RGBA, tex_w, tex_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &tex_data[0]);

  GLuint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *) &program);
  glUniform2i (glGetUniformLocation(program, "tex_size"), tex_w, tex_h);
  glUniform1i (glGetUniformLocation(program, "tex"), 0);
  glActiveTexture (GL_TEXTURE0);

  return texture;
}

#define IS_INSIDE_NO     0
#define IS_INSIDE_YES    1
#define IS_INSIDE_UNSURE 2

static GLuint
create_program (void)
{
  GLuint vshader, fshader, program;
  vshader = COMPILE_SHADER (GL_VERTEX_SHADER,
    attribute vec4 a_position;
    attribute vec2 a_texCoord;
    uniform mat4 u_matViewProjection;
    varying vec2 p;
    void main()
    {
      gl_Position = u_matViewProjection * a_position;
      p = a_texCoord;
    }
  );
  fshader = COMPILE_SHADER (GL_FRAGMENT_SHADER,
    uniform sampler2D tex;
    uniform ivec2 tex_size;

    varying vec2 p;

    vec2 perpendicular (const vec2 v) { return vec2 (-v.g, v.r); }
    int mod (const int a, const int b) { return a - (a / b) * b; }
    int div (const int a, const int b) { return a / b; }
    int floatToByte (const float v) { return int (v * (256 - 1e-5)); }

    vec3 arc_decode (const vec4 v)
    {
      float x = (float (mod (floatToByte (v.a) / 16, 16)) + v.r) / 16;
      float y = (float (mod (floatToByte (v.a)     , 16)) + v.g) / 16;
      float d = v.b;
      d = MAX_D * (2 * d - 1);
      return vec3 (x, y, d);
    }

    ivec2 rgba_to_pair (const vec4 v)
    {
      int x = (floatToByte (v.r) * 256 + floatToByte (v.g)) * 256 + floatToByte (v.b);
      int y = floatToByte (v.a);
      return ivec2 (x, y);
    }

    vec4 tex_1D (const sampler2D tex, int i)
    {
      return texture2D (tex, vec2 ((mod (i, tex_size.x) + .5) / float (tex_size.x),
				   (div (i, tex_size.x) + .5) / float (tex_size.y)));
    }

    void main()
    {
//      float m = length (vec2 (float (dFdy (p)), float (dFdx (p)))); /* isotropic antialiasing */
      float m = float (fwidth (p));//length (vec2 (float (dFdy (p)), float (dFdx (p)))); /* isotropic antialiasing */

      int p_cell_x = int (clamp (p.x, 0., 1.-1e-5) * GRID_X);
      int p_cell_y = int (clamp (p.y, 0., 1.-1e-5) * GRID_Y);

      ivec2 arc_position_data = rgba_to_pair (tex_1D (tex, p_cell_y * GRID_X + p_cell_x));
      int offset = arc_position_data.x;
      int num_endpoints =  div (arc_position_data.y, 2);
      int is_inside = mod (arc_position_data.y, 2);

      int i;
      float min_dist = 1.;
      float min_extended_dist = 1.;


      struct {
        vec2 p0;
	vec2 p1;
	vec2 c;
	float d;
      } closest_arc;
      struct {
        vec2 p0;
	vec2 p1;
	vec2 c;
	float d;
      } new_arc;

      vec3 arc_prev = vec3 (0,0,0);
      float min_point_dist = 1;
      for (i = 0; i < num_endpoints; i++)
      {
	vec3 arc = arc_decode (tex_1D (tex, i + offset));
	vec2 p0 = arc_prev.rg;
	arc_prev = arc;
	float d = arc.b;
	vec2 p1 = arc.rg;

	if (d == -MAX_D) continue;
	if (abs (d) < 1e-5) d = 1e-5; // cheat
	// find arc center
	vec2 line = p1 - p0;
	vec2 perp = perpendicular (line);
	vec2 c = mix (p0, p1, .5) - perp * ((1 - d*d) / (4 * d));

	// for highlighting points
	min_point_dist = min (min_point_dist, distance (p, p1));

	// unsigned distance
	float dist;
	if (sign (d) * dot (p - c, perpendicular (p0 - c)) <= 0 &&
	    sign (d) * dot (p - c, perpendicular (p1 - c)) >= 0)
	{
	  float signed_dist = distance (p, c) - distance (p0, c);
	  dist = abs (signed_dist);
	  if (dist <= min_dist) {
	    min_dist = dist;
	    is_inside = (sign (d) * sign (signed_dist)) < 0 ? IS_INSIDE_YES : IS_INSIDE_NO;
	  }
	} else {
	  dist = min (distance (p, p0), distance (p, p1));

	  if (dist < min_dist) {
	    min_dist = dist;
	    is_inside = IS_INSIDE_UNSURE;
	    closest_arc.p0 = p0;
	    closest_arc.p1 = p1;
	    closest_arc.c  = c;
	    closest_arc.d  = d;
	  } else if (dist == min_dist && is_inside == IS_INSIDE_UNSURE) {
	    // If this new distance is the same as the current minimum, compare extended distances.
	    // Take the sign from the arc with larger extended distance.
	    float extended_dist;
	    float old_extended_dist;
	    float new_extended_dist;

	    new_arc.p0 = p0;
	    new_arc.p1 = p1;
	    new_arc.c  = c;
	    new_arc.d  = d;
	    if (sign (new_arc.d) * dot (p - new_arc.c, perpendicular (new_arc.p0 - new_arc.c)) > 0)
	      extended_dist = - sign (new_arc.d) *  dot (p - new_arc.p0, normalize (new_arc.c - new_arc.p0));
	    else if (sign (new_arc.d) * dot (p - new_arc.c, perpendicular (new_arc.p1 - new_arc.c)) < 0)
	      extended_dist = - sign (new_arc.d) *  dot (p - new_arc.p1, normalize (new_arc.c - new_arc.p1));
	    else {
	      // XXX debug; not_reach
	      gl_FragColor = vec4(0,1,0,1);
	      return;
	    }
	    new_extended_dist = extended_dist;

	    new_arc.p0 = closest_arc.p0;
	    new_arc.p1 = closest_arc.p1;
	    new_arc.c  = closest_arc.c;
	    new_arc.d  = closest_arc.d;
	    if (sign (new_arc.d) * dot (p - new_arc.c, perpendicular (new_arc.p0 - new_arc.c)) > 0)
	      extended_dist = - sign (new_arc.d) *  dot (p - new_arc.p0, normalize (new_arc.c - new_arc.p0));
	    else if (sign (new_arc.d) * dot (p - new_arc.c, perpendicular (new_arc.p1 - new_arc.c)) < 0)
	      extended_dist = - sign (new_arc.d) *  dot (p - new_arc.p1, normalize (new_arc.c - new_arc.p1));
	    else {
	      // XXX debug; not_reach
	      gl_FragColor = vec4(1,1,0,1);
	      return;
	    }
	    old_extended_dist = extended_dist;

	    if (abs (new_extended_dist) > abs (old_extended_dist)) {
//	      min_dist = abs (new_extended_dist);
	      is_inside = old_extended_dist < 0 ? IS_INSIDE_YES : IS_INSIDE_NO;
	    } else {
//	      min_dist = abs (old_extended_dist);
	      is_inside = new_extended_dist < 0 ? IS_INSIDE_YES : IS_INSIDE_NO;
	    }

	  }
	}
      }

      min_dist *= is_inside == IS_INSIDE_YES ? -1 : +1;


      gl_FragColor = mix(vec4(1,0,0,1),
			 vec4(0,1,0,1) * ((1 + sin (min_dist / m))) * sin (pow (min_dist, .8) * M_PI),
			 smoothstep (0, 2 * m, abs (min_dist)));
      gl_FragColor = mix(vec4(0,1,0,1),
			 gl_FragColor,
			 smoothstep (.002, .005, min_point_dist));

      gl_FragColor += vec4(0,0,1,1) * num_endpoints * 16./255.;

      gl_FragColor += vec4(.5,0,0,1) * smoothstep (-m, m, -min_dist);

   //   gl_FragColor = vec4(1,1,1,1) * smoothstep (-m, m, min_dist);

    }
  );
  program = link_program (vshader, fshader);
  return program;
}

static int step_timer;
static int num_frames;

void display( void )
{
  int viewport[4];
  glGetIntegerv (GL_VIEWPORT, viewport);
  GLuint width  = viewport[2];
  GLuint height = viewport[3];

  double theta = M_PI / 360.0 * step_timer / 3.;
  GLfloat mat[] = { +cos(theta), +sin(theta), 0., 0.,
		    -sin(theta), +cos(theta), 0., 0.,
			     0.,          0., 1., 0.,
			     0.,          0., 0., 1., };

  glClearColor (0, 0, 0, 1);
  glClear (GL_COLOR_BUFFER_BIT);

  GLuint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *) &program);
  glUniformMatrix4fv (glGetUniformLocation (program, "u_matViewProjection"), 1, GL_FALSE, mat);

  glDrawArrays (GL_TRIANGLE_FAN, 0, 4);

  glutSwapBuffers ();
}

void
reshape (int width, int height)
{
  glViewport (0, 0, width, height);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, width, 0, height, -1, 1);
  glMatrixMode (GL_MODELVIEW);
  glutPostRedisplay ();
}

void keyboard( unsigned char key, int x, int y )
{
  switch (key) {
    case '\033': exit (0);
  }
}

static void
timed_step (int ms)
{
  glutTimerFunc (ms, timed_step, ms);
  num_frames++;
  step_timer++;
  glutPostRedisplay ();
}

static void
idle_step (void)
{
  glutIdleFunc (idle_step);
  num_frames++;
  step_timer++;
  glutPostRedisplay ();
}

static void
print_fps (int ms)
{
  glutTimerFunc (ms, print_fps, ms);
  printf ("%gfps\n", num_frames / 5.);
  num_frames = 0;
}

int
main (int argc, char** argv)
{
  char *font_path;
  char utf8;
  gboolean animate = FALSE;
  if (argc >= 3) {
     font_path = argv[1];
     utf8 = argv[2][0];
     if (argc >= 4)
       animate = atoi (argv[3]);
  }
  else
    die ("Usage: grid PATH_TO_FONT_FILE CHARACTER_TO_DRAW ANIMATE?");

  glutInit (&argc, argv);
  glutInitWindowSize (512, 512);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("GLyphy");
  glutReshapeFunc (reshape);
  glutDisplayFunc (display);
  glutKeyboardFunc (keyboard);

  glewInit ();
  if (!glewIsSupported ("GL_VERSION_2_0"))
    die ("OpenGL 2.0 not supported");

  GLuint program, texture, a_pos_loc, a_tex_loc;


  program = create_program ();
  glUseProgram (program);

  texture = create_texture (font_path, utf8);

  a_pos_loc = glGetAttribLocation(program, "a_position");
  a_tex_loc = glGetAttribLocation(program, "a_texCoord");
  const GLfloat w_vertices[] = { -1, -1, 0,  -0.1, -0.1,
				 +1, -1, 0,  1.1, -0.1,
				 +1, +1, 0,  1.1, 1.1,
				 -1, +1, 0,  -0.1, 1.1 };
  glVertexAttribPointer (a_pos_loc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), w_vertices+0);
  glVertexAttribPointer (a_tex_loc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), w_vertices+3);
  glEnableVertexAttribArray (a_pos_loc);
  glEnableVertexAttribArray (a_tex_loc);

  if (animate) {
    //glutTimerFunc (40, timed_step, 40);
    glutIdleFunc (idle_step);
    glutTimerFunc (5000, print_fps, 5000);
  }

  glutMainLoop ();

  return 0;
}
