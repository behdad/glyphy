#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include <GLES2/gl2.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

#include <cairo.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <assert.h>
#include <cairo-ft.h>
#include <string>
#include <list>

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
#define COMPILE_SHADER1(Type,Src) compile_shader (Type, "#version 130\n" #Src)
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


static void
create_egl_for_drawable (EGLDisplay edpy, GdkDrawable *drawable, EGLSurface *surface, EGLContext *context)
{
  EGLConfig econfig;
  EGLint num_configs;
  const EGLint attribs[] = {
    EGL_BUFFER_SIZE, 32,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, GDK_IS_WINDOW (drawable) ? EGL_WINDOW_BIT : EGL_PIXMAP_BIT,
    EGL_NONE
  };
  const EGLint ctx_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  if (!eglChooseConfig(edpy, attribs, &econfig, 1, &num_configs) || !num_configs)
    die ("Could not find EGL config");

  if (!(*surface = eglCreateWindowSurface (edpy, econfig, GDK_DRAWABLE_XID (drawable), NULL)))
    die ("Could not create EGL surface");

  if (!(*context = eglCreateContext (edpy, econfig, EGL_NO_CONTEXT, ctx_attribs)))
    die ("Could not create EGL context");
}

static GQuark
drawable_egl_quark (void)
{
  static GQuark quark = 0;
  if (G_UNLIKELY (!quark))
    quark = g_quark_from_string ("egl_drawable");
  return quark;
}

typedef struct {
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;
} drawable_egl_t;

static void
drawable_egl_destroy (drawable_egl_t *e)
{
  eglDestroyContext (e->display, e->context);
  eglDestroySurface (e->display, e->surface);
  g_slice_free (drawable_egl_t, e);
}

static drawable_egl_t *
drawable_get_egl (GdkDrawable *drawable)
{
  drawable_egl_t *e;

  if (G_UNLIKELY (!(e = (drawable_egl_t *) g_object_get_qdata ((GObject *) drawable, drawable_egl_quark ())))) {
    e = g_slice_new (drawable_egl_t);
    e->display = eglGetDisplay (GDK_DRAWABLE_XDISPLAY (drawable));
    create_egl_for_drawable (e->display, drawable, &e->surface, &e->context);
    g_object_set_qdata_full (G_OBJECT (drawable), drawable_egl_quark (), e, (GDestroyNotify) drawable_egl_destroy);
  }

  return e;
}

static void
drawable_make_current (GdkDrawable *drawable)
{
  drawable_egl_t *e = drawable_get_egl (drawable);
  eglMakeCurrent(e->display, e->surface, e->surface, e->context);
}

static void
drawable_swap_buffers (GdkDrawable *drawable)
{
  drawable_egl_t *e = drawable_get_egl (drawable);
  eglSwapBuffers (e->display, e->surface);
  glFinish ();
}








/* TODO Knobs */
#define MIN_FONT_SIZE 1
#define GRID_SIZE 32
#define GRID_X GRID_SIZE
#define GRID_Y GRID_SIZE
#define TOLERANCE 3e-5







/* Given a cell, fills the vector closest_arcs with arcs that may be closest to some point in the cell.
 * Uses idea that all close arcs to cell must be ~close to center of cell.
 */
static int
closest_arcs_to_cell (Point<Coord> square_top_left,
		      Scalar cell_width,
		      Scalar cell_height,
		      Scalar grid_size,
		      vector<arc_t> arc_list,
		      vector<arc_t> &near_arcs)
{
  // Find distance between cell center and cell's closest arc.
  point_t center (square_top_left.x + cell_width / 2.,
                  square_top_left.y + cell_height / 2.);
  double min_distance = INFINITY;

  for (int k = 0; k < arc_list.size (); k++) {
    arc_t arc = arc_list [k];
    double current_distance = fabs (arc.distance_to_point (center));

    if (current_distance < min_distance) {
      min_distance = current_distance;
    }
  }

  // If d is the distance from the center of the square to the nearest arc, then
  // all nearest arcs to the square must be at most [d + s/sqrt(2)] from the center.
  double half_diagonal = sqrt (cell_height * cell_height + cell_width * cell_width) / 1.4;
  Scalar radius = min_distance + half_diagonal;

  double faraway = double (grid_size) / MIN_FONT_SIZE;
  if (min_distance - half_diagonal <= faraway)
    for (int k = 0; k < arc_list.size (); k++) {
      arc_t current_arc = arc_list [k];
      if (fabs(current_arc.distance_to_point (center)) <= radius)
        near_arcs.push_back (current_arc);
    }
}



/* Bit packing */

#define UPPER_BITS(v,bits,total_bits) ((v) >> ((total_bits) - (bits)))
#define LOWER_BITS(v,bits,total_bits) ((v) & ((1 << (bits)) - 1))

struct rgba_t {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};


#define ARC_ENCODE_X_BITS 12
#define ARC_ENCODE_Y_BITS 12
#define ARC_ENCODE_D_BITS (32 - ARC_ENCODE_X_BITS - ARC_ENCODE_Y_BITS)
#define ARC_ENCODE_X_CHANNEL r
#define ARC_ENCODE_Y_CHANNEL g
#define ARC_ENCODE_D_CHANNEL b
#define ARC_ENCODE_OTHER_CHANNEL a

G_STATIC_ASSERT (8 <= ARC_ENCODE_X_BITS && ARC_ENCODE_X_BITS < 16);
G_STATIC_ASSERT (8 <= ARC_ENCODE_Y_BITS && ARC_ENCODE_Y_BITS < 16);
G_STATIC_ASSERT (8 <= ARC_ENCODE_D_BITS && ARC_ENCODE_D_BITS <= 16);
G_STATIC_ASSERT (ARC_ENCODE_X_BITS + ARC_ENCODE_Y_BITS + ARC_ENCODE_D_BITS <= 32);

static const rgba_t
arc_encode (double x, double y, double d)
{
  rgba_t v;

  // lets do 10 bits for d, and 11 for x and y each 
  unsigned int ix, iy, id;
  ix = lround (x * ((1 << ARC_ENCODE_X_BITS) - 1));
  g_assert (ix < (1 << ARC_ENCODE_X_BITS));
  iy = lround (y * ((1 << ARC_ENCODE_Y_BITS) - 1));
  g_assert (iy < (1 << ARC_ENCODE_Y_BITS));
#define MAX_D .54 // TODO (0.25?)
  if (isinf (d))
    id = 0;
  else {
    g_assert (fabs (d) < MAX_D);
    id = lround (d * ((1 << (ARC_ENCODE_D_BITS - 1)) - 1) / MAX_D + (1 << (ARC_ENCODE_D_BITS - 1)));
  }
  g_assert (id < (1 << ARC_ENCODE_D_BITS));

  v.ARC_ENCODE_X_CHANNEL = LOWER_BITS (ix, 8, ARC_ENCODE_X_BITS);
  v.ARC_ENCODE_Y_CHANNEL = LOWER_BITS (iy, 8, ARC_ENCODE_Y_BITS);
  v.ARC_ENCODE_D_CHANNEL = UPPER_BITS (id, 8, ARC_ENCODE_D_BITS);
  v.ARC_ENCODE_OTHER_CHANNEL = ((ix >> 8) << (ARC_ENCODE_Y_BITS - 8 + ARC_ENCODE_D_BITS - 8))
			     | ((iy >> 8) << (ARC_ENCODE_D_BITS - 8))
			     | (id & ((1 << (ARC_ENCODE_D_BITS - 8)) - 1));
  return v;
}


static rgba_t
pair_to_rgba (unsigned int num1, unsigned int num2)
{
  rgba_t v;
  v.r = UPPER_BITS (num1, 8, 16);
  v.g = LOWER_BITS (num1, 8, 16);
  v.b = UPPER_BITS (num2, 8, 16);
  v.a = LOWER_BITS (num2, 8, 16);
  return v;
}



static GLint
create_texture (const char *font_path, const char UTF8, GLint program)
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
  int grid_min_x, grid_max_x, grid_min_y, grid_max_y, glyph_width, glyph_height;
  grid_min_x = face->glyph->metrics.horiBearingX;
  grid_min_y = face->glyph->metrics.horiBearingY - face->glyph->metrics.height;
  grid_max_x = face->glyph->metrics.horiBearingX + face->glyph->metrics.width;
  grid_max_y = face->glyph->metrics.horiBearingY;

  glyph_width = grid_max_x - grid_min_x;
  glyph_height = grid_max_y - grid_min_y;

  double box_width = glyph_width / GRID_X;
  double box_height = glyph_height / GRID_Y;




  // Make a 2d grid for arc/cell information.
  vector<rgba_t> tex_data;

  // near_arcs: Vector of arcs near points in this single grid cell
  vector<arc_t> near_arcs;

  double min_dimension = std::min(glyph_width, glyph_height);
  unsigned int header_length = GRID_X * GRID_Y;
  unsigned int offset = header_length;
  tex_data.resize (header_length);
  for (int row = 0; row < GRID_Y; row++)
    for (int col = 0; col < GRID_X; col++)
    {
      near_arcs.clear ();
      closest_arcs_to_cell (Point<Coord> (grid_min_x + (col * box_width), grid_min_y + (row * box_height)),
                            box_width, box_height, min_dimension, acc.arcs, near_arcs);

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

      tex_data[row * GRID_X + col] = pair_to_rgba (offset, tex_data.size () - offset);
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

  GLint tex_size[2] = {tex_w, tex_h};
  glUniform2i (glGetUniformLocation(program, "tex_size"), tex_w, tex_h);
  glUniform1i (glGetUniformLocation(program, "tex"), 0);
  glActiveTexture (GL_TEXTURE0);
}

static GLuint
create_program (void)
{
  GLuint vshader, fshader, program;
  vshader = COMPILE_SHADER (GL_VERTEX_SHADER,
      attribute vec4 a_position;
      attribute vec2 a_texCoord;
      uniform mat4 u_matViewProjection;
      invariant varying vec2 p;
      void main()
      {
	gl_Position = u_matViewProjection * a_position;
	p = a_texCoord;
      }
  );
  fshader = COMPILE_SHADER (GL_FRAGMENT_SHADER,
      uniform highp sampler2D tex;
      uniform ivec2 tex_size;

      invariant varying highp vec2 p;

      vec2 perpendicular (const vec2 v) { return vec2 (-v.g, v.r); }
      vec2 projection (const vec2 v, const vec2 base) { 
        return length (base) < 1e-5 ? vec2 (0,0) :
                  float (dot (v, base)) / float (dot (base, base)) * base;
      } 
      int mod (const int a, const int b) { return a - (a / b) * b; }
      int div (const int a, const int b) { return a / b; }

      vec3 arc_decode (const vec4 v)
      {
	float x = (float (mod (int (v.ARC_ENCODE_OTHER_CHANNEL * (256-1e-5)) / (1 << (ARC_ENCODE_Y_BITS - 8 + ARC_ENCODE_D_BITS - 8)), (1 << (ARC_ENCODE_X_BITS - 8)))) +
		   v.ARC_ENCODE_X_CHANNEL) / (1 << (ARC_ENCODE_X_BITS - 8));
	float y = (float (mod (int (v.ARC_ENCODE_OTHER_CHANNEL * (256-1e-5)) / (1 << (ARC_ENCODE_D_BITS - 8)), (1 << (ARC_ENCODE_Y_BITS - 8)))) +
		   v.ARC_ENCODE_Y_CHANNEL) / (1 << (ARC_ENCODE_Y_BITS - 8));
	float d = v.ARC_ENCODE_D_CHANNEL + float (mod (int (v.ARC_ENCODE_OTHER_CHANNEL * (256-1e-5)), (1 << (ARC_ENCODE_D_BITS - 8)))) / (1 << 8);
	d = MAX_D * (2 * d - 1);
	return vec3 (x, y, d);
      }

      ivec2 rgba_to_pair (const vec4 v)
      {
        int x = int ((256-1e-5) * v.r) * 256 + int ((256-1e-5) * v.g);
        int y = int ((256-1e-5) * v.b) * 256 + int ((256-1e-5) * v.a);
        return ivec2 (x, y);
      }

      vec4 tex_1D (const sampler2D tex, int i)
      {
	return texture2D (tex, vec2 ((mod (i, tex_size.x) + .5) / float (tex_size.x),
				     (div (i, tex_size.x) + .5) / float (tex_size.y)));
      }

      void main()
      {
	float m = length (vec2 (float (dFdy (p)), float (dFdx (p)))); /* isotropic antialiasing */
		
	int p_cell_x = int (clamp (p.x, 0., 1.-1e-5) * GRID_X);
	int p_cell_y = int (clamp (p.y, 0., 1.-1e-5) * GRID_Y);

	ivec2 arc_position_data = rgba_to_pair(tex_1D (tex, p_cell_y * GRID_X + p_cell_x));
	int offset = arc_position_data.x;
	int num_endpoints =  arc_position_data.y;

	int i;
	float min_dist = 1.;
	float min_point_dist = 1.;
	float min_extended_dist = 1.;
	bool is_inside = false;
	
	vec3 arc_prev = vec3 (0,0, 0);
	for (i = 0; i < num_endpoints; i++)
	{
	  vec3 arc = arc_decode (tex_1D (tex, i + offset));
	  vec2 p0 = arc_prev.rg;
	  arc_prev = arc;
	  float d = arc.b;
	  if (d == -MAX_D) continue;
	  if (abs (d) < 1e-5) d = 1e-5; // cheat
	  vec2 p1 = arc.rg;
	  vec2 line = p1 - p0;
	  vec2 perp = perpendicular (line);
	  vec2 norm = normalize (perp);
	  vec2 c = mix (p0, p1, .5) - perp * ((1 - d*d) / (4 * d));

	  // Find the distance from p to the nearby arcs.
	  float dist;
	  if (sign (d) * dot (p - c, perpendicular (p0 - c)) <= 0 &&
	      sign (d) * dot (p - c, perpendicular (p1 - c)) >= 0)
	  {
	    dist = abs (distance (p, c) - distance (p0, c));
	  } else {
	    dist = min (distance (p, p0), distance (p, p1));
	  }
	  
	  // If this new distance is roughly the same as the current minimum, compare extended distances.
	  // Take the sign from the arc with larger extended distance.
	  if (abs(dist - min_dist) < 1e-6) {
	    float extended_dist;
	    
	    if (sign (d) * dot (p - c, perpendicular (p0 - c)) <= 0 &&
	        sign (d) * dot (p - c, perpendicular (p1 - c)) >= 0) {
	      extended_dist = dist;
	    } 
	    else if (sign (d) * dot (p - c, perpendicular (p0 - c)) > 0) {
	      extended_dist = length (projection (p - p0, p0 - c)); 
	    }
	    else if (sign (d) * dot (p - c, perpendicular (p1 - c)) < 0) {
	      extended_dist = length (projection (p - p1, p1 - c));
	    }
	    
	    if (extended_dist > min_extended_dist) {
	      min_extended_dist = extended_dist;
	      if ((sign (d) > 0 && distance (p, c) <= distance (p0, c)) ||
	          (sign (d) < 0 && distance (p, c) >= distance (p0, c)))
	        is_inside = true;
	      else
	        is_inside = false;
	    }
	  }
	  
	  else if (dist < min_dist) {
	    min_dist = dist;
	    
	    // Get the new minimum extended distance.
	    if (sign (d) * dot (p - c, perpendicular (p0 - c)) <= 0 &&
	        sign (d) * dot (p - c, perpendicular (p1 - c)) >= 0) {
	      min_extended_dist = dist;
	    } 
	    else if (sign (d) * dot (p - c, perpendicular (p0 - c)) > 0) {
	      min_extended_dist = length (projection (p - p0, p0 - c)); 
	    }
	    else if (sign (d) * dot (p - c, perpendicular (p1 - c)) < 0) {
	      min_extended_dist = length (projection (p - p1, p1 - c));
	    }
	    
	    if ((distance (p, c) <= distance (p0, c) && sign (d) > 0) ||
	        (distance (p, c) >= distance (p0, c) && sign (d) < 0))
	      is_inside = true;
	    else
	      is_inside = false;
	  }
	  

	  float point_dist = min (distance (p, p0), distance (p, p1));
	  min_point_dist = min (min_point_dist, point_dist);
	}
	
	 gl_FragColor = mix(vec4(1,0,0,1),
			   vec4(0,1,0,1) * ((1 + sin (min_dist / m))) * sin (pow (min_dist, .8) * M_PI),
			   smoothstep (0, 2 * m, min_dist));
	 gl_FragColor = mix(vec4(0,1,0,1),
			   gl_FragColor,
			   smoothstep (.002, .005, min_point_dist));
	gl_FragColor += vec4(0,0,1,1) * num_endpoints / 16;
	gl_FragColor += vec4(.5,0,0,1) * smoothstep (-m, m, is_inside ? min_dist : -min_dist);
	
	//gl_FragColor = vec4(1,1,1,1) * smoothstep (-m, m, is_inside ? -min_dist : min_dist);
	return;
    }
  );
  program = link_program (vshader, fshader);
  return program;
}


static
gboolean configure_cb (GtkWidget *widget,
		       GdkEventConfigure *event,
		       gpointer user_data)
{
  gdk_window_invalidate_rect (widget->window, NULL, TRUE);
  return FALSE;
}

static int step_timer;
static int num_frames;

static
gboolean expose_cb (GtkWidget *widget,
		    GdkEventExpose *event,
		    gpointer user_data)
{
  GtkAllocation allocation;
  double theta = M_PI / 360.0 * step_timer / 3.;
  GLfloat mat[] = { +cos(theta), +sin(theta), 0., 0.,
		    -sin(theta), +cos(theta), 0., 0.,
			     0.,          0., 1., 0.,
			     0.,          0., 0., 1., };

  drawable_make_current (widget->window);

  gtk_widget_get_allocation (widget, &allocation);
  glViewport(0, 0, allocation.width, allocation.height);
  glClearColor(1., 1., 0., 0.);
  glClear(GL_COLOR_BUFFER_BIT);
  glUniformMatrix4fv (GPOINTER_TO_INT (user_data), 1, GL_FALSE, mat);

  glDrawArrays (GL_TRIANGLE_FAN, 0, 4);

  drawable_swap_buffers (widget->window);

  return TRUE;
}

static gboolean
step (gpointer data)
{
  num_frames++;
  step_timer++;
  gdk_window_invalidate_rect (GDK_WINDOW (data), NULL, TRUE);
  return TRUE;
}

static gboolean
print_fps (gpointer data)
{
  printf ("%gfps\n", num_frames / 5.);
  num_frames = 0;
  return TRUE;
}

int
main (int argc, char** argv)
{
  GtkWidget *window;
  char *font_path;
  char utf8;
  gboolean animate = FALSE;
  if (argc >= 3) {
     font_path = argv[1];
     utf8 = argv[2][0];
     if (argc >= 4)
       animate = atoi (argv[3]);
  }
  else {
    fprintf (stderr, "Usage: grid PATH_TO_FONT_FILE CHARACTER_TO_DRAW ANIMATE?\n");
    return 1;
  }

  GLuint program, texture, a_pos_loc, a_tex_loc;

  const GLfloat w_vertices[] = { -1, -1, 0,  -0.1, -0.1,
				 +1, -1, 0,  1.1, -0.1,
				 +1, +1, 0,  1.1, 1.1,
				 -1, +1, 0,  -0.1, 1.1 };

  gtk_init (&argc, &argv);

  window = GTK_WIDGET (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  gtk_window_set_default_size (GTK_WINDOW (window), 600, 600);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  eglInitialize (eglGetDisplay (gdk_x11_display_get_xdisplay (gtk_widget_get_display (window))), NULL, NULL);
  if (!eglBindAPI (EGL_OPENGL_ES_API))
    die ("Failed to bind OpenGL ES API");

  gtk_widget_show_all (window);

  drawable_make_current (window->window);

  program = create_program ();
  glUseProgram (program);

  texture = create_texture (font_path, utf8, program);

  a_pos_loc = glGetAttribLocation(program, "a_position");
  a_tex_loc = glGetAttribLocation(program, "a_texCoord");

  glVertexAttribPointer (a_pos_loc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), w_vertices+0);
  glVertexAttribPointer (a_tex_loc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), w_vertices+3);
  glEnableVertexAttribArray (a_pos_loc);
  glEnableVertexAttribArray (a_tex_loc);

  gtk_widget_set_double_buffered (window, FALSE);
  gtk_widget_set_redraw_on_allocate (window, TRUE);
  /* TODO, the uniform location can change if we recompile program. */
  g_signal_connect (G_OBJECT (window), "expose-event", G_CALLBACK (expose_cb),
		    GINT_TO_POINTER (glGetUniformLocation (program, "u_matViewProjection")));
  g_signal_connect (G_OBJECT (window), "configure-event", G_CALLBACK (configure_cb), NULL);

  if (animate) {
    g_idle_add (step, window->window);
    g_timeout_add (5000, print_fps, NULL);
  }

  gtk_main ();

  return 0;
}
