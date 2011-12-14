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


static double min_font_size = 10; /************************************************************************ ARBITRARY *********************************/
static const int NUM_SAVED_ARCS = 20;  /********************************************************************Too high **********************************/

struct grid_cell {
  unsigned short arcs [NUM_SAVED_ARCS]; /******************************************************* char? unsigned char? *******************************/
  char more_arcs_pointer;
  bool inside_glyph;
};

//#define TEXSIZE 32  /*************************************************************************** 32? 64? Higher? Lower? Non-constant? **************/
//struct arcs_texture  /********************************************************************************* Struct or Class? **************************/
//{  
//    std::vector<point_t> arc_endpoints;
//    std::vector<double>  d_values;    
//    grid_cell grid [TEXSIZE][TEXSIZE];  /********************************************************** 32 by 32? 64 by 64? ***************************/
//};


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
create_program (GLuint vshader, GLuint fshader)
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


 
/** Given a cell, fills the vector closest_arcs with arcs that may be closest to some point in the cell.
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
  arc_t nearest_arc = arc_list [0];
  double distance = min_distance;

  for (int k = 0; k < arc_list.size (); k++) {
    arc_t current_arc = arc_list [k];
    double current_distance = current_arc.distance_to_point (center);    

    // If two arcs are equally close to this point, take the sign from the one whose extension is farther away. 
    // (Extend arcs using tangent lines from endpoints; this is done using the SignedVector operation "-".) 
    if (fabs (fabs (current_distance) - fabs(min_distance)) < 1e-6) { 
      SignedVector<Coord> to_arc_min = nearest_arc - center;
      SignedVector<Coord> to_arc_current = current_arc - center;      
      if (to_arc_min.len () < to_arc_current.len ()) {
        min_distance = fabs (min_distance) * (to_arc_current.negative ? -1 : 1);
      }
    }
    else if (fabs (current_distance) < fabs(min_distance)) {
      min_distance = current_distance;
      nearest_arc = current_arc;
    }
  }
  
    
  // If d is the distance from the center of the square to the nearest arc, then
  // all nearest arcs to the square must be at most [d + s/sqrt(2)] from the center. 
  min_distance = fabs (min_distance);
  double half_diagonal = sqrt(cell_height * cell_height + cell_width * cell_width) / 2;
  Scalar radius = min_distance + half_diagonal;
 // printf("Minimum distance is %g. ", min_distance);

  double tolerance = grid_size / min_font_size; 
 
  if (min_distance - half_diagonal <= tolerance) 
    for (int k = 0; k < arc_list.size (); k++) {
      arc_t current_arc = arc_list [k];
      if (fabs(current_arc.distance_to_point (center)) < radius) {
 //         printf("%g (%d), ", current_arc.distance_to_point (center), k);
        near_arcs.push_back (current_arc);     
      }      
    }    
//   printf("\n  Array of indices made:");
    
//  for (int k = 0; k < near_arcs.size(); k++)
//    printf("%g ", near_arcs[k].d);
//  printf("\n");
}




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
#define UPPER_BITS(v,bits,total_bits) ((v) >> ((total_bits) - (bits)))
#define LOWER_BITS(v,bits,total_bits) ((v) & ((1 << (bits)) - 1))

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
/*
  v.r = ix & 0xff;
  v.g = iy & 0xff;
  v.b = id >> (ARC_ENCODE_D_BITS - 8);
  v.a = ((ix >> 8) << (ARC_ENCODE_Y_BITS - 8 + ARC_ENCODE_D_BITS - 8))
      | ((iy >> 8) << (ARC_ENCODE_D_BITS - 8))
      | (id & ((1 << (ARC_ENCODE_D_BITS - 8)) - 1)); */
  v.ARC_ENCODE_X_CHANNEL = LOWER_BITS (ix, 8, ARC_ENCODE_X_BITS);
  v.ARC_ENCODE_Y_CHANNEL = LOWER_BITS (iy, 8, ARC_ENCODE_Y_BITS);
  v.ARC_ENCODE_D_CHANNEL = UPPER_BITS (id, 8, ARC_ENCODE_D_BITS);
  v.ARC_ENCODE_OTHER_CHANNEL = ((ix >> 8) << (ARC_ENCODE_Y_BITS - 8 + ARC_ENCODE_D_BITS - 8))
			     | ((iy >> 8) << (ARC_ENCODE_D_BITS - 8))
			     | (id & ((1 << (ARC_ENCODE_D_BITS - 8)) - 1));
  return v;
}


/*

static const rgba_t
arc_encodej (double x, double y, double d)
{
  rgba_t v;

  // lets do 10 bits for d, and 11 for x and y each 
  unsigned int ix, iy, id;
  ix = lround (x * (1 << (ARC_ENCODE_X_BITS - 1)));
  g_assert (ix < (1 << ARC_ENCODE_X_BITS));
  iy = lround (y * (1 << (ARC_ENCODE_Y_BITS - 1)));
  g_assert (iy < (1 << ARC_ENCODE_Y_BITS));
#define MAX_D .54 // TODO
  if (isinf (d))
    id = 0;
  else {
    g_assert (fabs (d) < MAX_D);
    d = (d / (2 * MAX_D)) + 0.5; // in [0, 1]
    g_assert (0 <= d && d <= 1);
    id = lround (d * (1 << (ARC_ENCODE_D_BITS - 1)));
  }
  g_assert (id < (1 << ARC_ENCODE_D_BITS));
  v.r = ix & 0xff;
  v.g = iy & 0xff;
  v.b = id & 0xff;
  v.a = ((ix >> 8) << (ARC_ENCODE_Y_BITS - 8 + ARC_ENCODE_D_BITS - 8))
      | ((iy >> 8) << (ARC_ENCODE_D_BITS - 8))
      | (id >> 8);
=======
  v.ARC_ENCODE_X_CHANNEL = LOWER_BITS (ix, 8, ARC_ENCODE_X_BITS);
  v.ARC_ENCODE_Y_CHANNEL = LOWER_BITS (iy, 8, ARC_ENCODE_Y_BITS);
  v.ARC_ENCODE_D_CHANNEL = UPPER_BITS (id, 8, ARC_ENCODE_D_BITS);
  v.ARC_ENCODE_OTHER_CHANNEL = ((ix >> 8) << (ARC_ENCODE_Y_BITS - 8 + ARC_ENCODE_D_BITS - 8))
			     | ((iy >> 8) << (ARC_ENCODE_D_BITS - 8))
			     | (id & ((1 << (ARC_ENCODE_D_BITS - 8)) - 1));
>>>>>>> d060cdad84179ca87ac3570c9c6d5c0f48c6fd5e
  return v;
}*/



static rgba_t 
pair_to_rgba_t (unsigned int num1, unsigned int num2)
{
  rgba_t v;
  v.r = (num1 & 0xff00) / 0x100;
  v.g = num1 & 0xff;
  v.b = (num2 & 0xff00) / 0x100;
  v.a = num2 & 0xff;
  return v;
}



/***************************************************************************************************************************/
/*** This uses the new SDF approach. ***************************************************************************************/
/***************************************************************************************************************************/
static void
setup_texture (const char *font_path, const char UTF8, GLint program)
{
  FT_Face face;
  FT_Library library;
  FT_Init_FreeType (&library);   
  FT_New_Face ( library, font_path, 0, &face );
  unsigned int upem = face->units_per_EM;
  double tolerance = upem * 1e-5; // in font design units


  // Arc approximation code.
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


  // The actual arc decomposition is done here.
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
  FreeTypeOutlineSource<ArcApproximatorOutlineSink>::decompose_outline (&face->glyph->outline,
  									outline_arc_approximator);
  printf ("Num arcs %d; Approximation error %g; Tolerance %g; Percentage %g. %s\n",
	  (int) acc.arcs.size (), e, tolerance, round (100 * e / tolerance), e <= tolerance ? "PASS" : "FAIL");

  int grid_min_x, grid_max_x, grid_min_y, grid_max_y, glyph_width, glyph_height;
  grid_min_x = face->glyph->metrics.horiBearingX;
  grid_min_y = face->glyph->metrics.horiBearingY - face->glyph->metrics.height;
  grid_max_x = face->glyph->metrics.horiBearingX + face->glyph->metrics.width;
  grid_max_y = face->glyph->metrics.horiBearingY;

  glyph_width = grid_max_x - grid_min_x;
  glyph_height = grid_max_y - grid_min_y;
  printf ("Glyph dimensions: [%d, %d] x [%d, %d]. Width = %d, height = %d.\n",
          grid_min_x, grid_max_x, grid_min_y, grid_max_y, glyph_width, glyph_height);
  
    
#define GRIDSIZE 32
  double box_width = glyph_width / GRIDSIZE;
  double box_height = glyph_height / GRIDSIZE;



  for (int i = 0; i < acc.arcs.size(); i++)
    printf("Arc: (%g, %g) to (%g, %g), d = %g.\n", acc.arcs[i].p0.x, acc.arcs[i].p0.y,  acc.arcs[i].p1.x, acc.arcs[i].p1.y, acc.arcs[i].d);


    // Make a 2d grid for arc/cell information.
  /**************************************************************************************************************************************************/

  // arc_data_vector: Vector of rgba_t objects storing data of near arcs for ALL cells in the grid. 
  //                  Contains duplicate entries. Used to translate to texture.
  vector<rgba_t> arc_data_vector;
  arc_data_vector.clear ();

  // num_endpoints: The (i*GRIDSIZE + j)th entry corresponds to the number of arcs near the (i, j)th cell in the grid.
  vector<int> num_endpoints;

  double min_dimension = std::min(glyph_width, glyph_height);
  for (int row = 0; row < GRIDSIZE; row++)
    for (int col = 0; col < GRIDSIZE; col++) {

      // near_arcs: Vector of arcs near points in this single grid cell
      vector<arc_t> near_arcs;
      near_arcs.clear ();

      // endpoints: Vector of endpoints of arcs in the vector near_arcs
      vector<point_t> endpoints;
      endpoints.clear ();

      // d_values: Vector of d values for arcs in the vector near_arcs
      vector<double> d_values;
      d_values.clear ();

      closest_arcs_to_cell (Point<Coord> (grid_min_x + (col * box_width), grid_min_y + (row * box_height)),
                              box_width, box_height, min_dimension, acc.arcs, near_arcs);

      int arc_counter;
      for (arc_counter = 0; arc_counter + 1 < near_arcs.size (); arc_counter++) {
        arc_t current_arc = near_arcs [arc_counter];
        endpoints.push_back (current_arc.p0);
        d_values.push_back (current_arc.d);
        // Start a new loop in the outline.
        if (current_arc.p1 != near_arcs [arc_counter+1].p0) {
          endpoints.push_back (current_arc.p1);
          d_values.push_back (INFINITY);
        }
      }

      // The last arc needs to be done specially.
      if (near_arcs.size () > 0) {
        endpoints.push_back (near_arcs [arc_counter].p0);
        endpoints.push_back (near_arcs [arc_counter].p1);
        d_values.push_back (near_arcs [arc_counter].d);
        d_values.push_back (INFINITY);
      }

      // Get arc endpoint data into an rgba_t vector.
      for (int i = 0; i < endpoints.size (); i++)
        arc_data_vector.push_back (arc_encode ((endpoints[i].x - grid_min_x) / glyph_width,
	  	 	                       (endpoints[i].y - grid_min_y) / glyph_height,
			                       d_values[i]));
      num_endpoints.push_back (endpoints.size ());
    }

  int header_length = num_endpoints.size ();
  int offset = header_length;
  rgba_t tex_array [header_length + arc_data_vector.size () ];

  for (int i = 0; i < header_length; i++) {
    tex_array [i] = pair_to_rgba_t (offset, num_endpoints [i]);
//    printf("(%d, %d) => (%d, %d, %d, %d) = (%d, %d).\n", offset, num_endpoints[i], tex_array[i].r, tex_array[i].g, tex_array[i].b, tex_array[i].a,
//     tex_array[i].r * 256 + tex_array[i].g, tex_array[i].b * 256 + tex_array[i].a);

    offset += num_endpoints [i];
  }
  for (int i = 0; i < arc_data_vector.size (); i++)
    tex_array [i + header_length] = arc_data_vector [i];

  gl(TexImage2D) (GL_TEXTURE_2D, 0, GL_RGBA, 1, header_length + arc_data_vector.size (), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_array);
  glUniform1i (glGetUniformLocation(program, "upem"), upem);
  glUniform1i (glGetUniformLocation(program, "texture_size"), header_length + arc_data_vector.size ());
  
  rgba_t input (pair_to_rgba_t(7728, 12));
  
  printf("Got %d, %d as %d, %d, %d, %d.\n", 7728, 12, input.r, input.g, input.b, input.a);
  glUniform1i (glGetUniformLocation(program, "inputr"), input.r);
  glUniform1i (glGetUniformLocation(program, "inputg"), input.g);
  glUniform1i (glGetUniformLocation(program, "inputb"), input.b);
  glUniform1i (glGetUniformLocation(program, "inputa"), input.a);
  
  return;

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
  printf ("%d frames per second\n", (num_frames + 2) / 5);
  num_frames = 0;
  return TRUE;
}

int
main (int argc, char** argv)
{

 // arc_decode( arc_encode(0.875, 0.466, 0.111154));
  




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

  GLuint vshader, fshader, program, texture, a_pos_loc, a_tex_loc;

  const GLfloat w_vertices[] = { -1, -1, 0,  0, 0,
				 +1, -1, 0,  1, 0,
				 +1, +1, 0,  1, 1,
				 -1, +1, 0,  0, 1 };

  gtk_init (&argc, &argv);

  window = GTK_WIDGET (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  gtk_window_set_default_size (GTK_WINDOW (window), 600, 600);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  eglInitialize (eglGetDisplay (gdk_x11_display_get_xdisplay (gtk_widget_get_display (window))), NULL, NULL);
  if (!eglBindAPI (EGL_OPENGL_ES_API))
    die ("Failed to bind OpenGL ES API");

  gtk_widget_show_all (window);

  drawable_make_current (window->window);

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
      uniform int upem;
      uniform int num_points;
      uniform int inputr;
      uniform int inputg;
      uniform int inputb;
      uniform int inputa;

      uniform int texture_size;

      invariant varying highp vec2 p;

      vec2 perpendicular (const vec2 v) { return vec2 (-v.g, v.r); }
      int mod (const int a, const int b) { return a - (a / b) * b; }

      vec3 arc_decode (const vec4 v)
      {
	//float x = v.ARC_ENCODE_X_CHANNEL;
	//float y = v.ARC_ENCODE_Y_CHANNEL;
	//float d = v.ARC_ENCODE_D_CHANNEL;
	float x = (float (mod (int (v.ARC_ENCODE_OTHER_CHANNEL * (256-1e-5)) / (1 << (ARC_ENCODE_Y_BITS - 8 + ARC_ENCODE_D_BITS - 8)), (1 << (ARC_ENCODE_X_BITS - 8)))) +
		   v.ARC_ENCODE_X_CHANNEL) / (1 << (ARC_ENCODE_X_BITS - 8));
	float y = (float (mod (int (v.ARC_ENCODE_OTHER_CHANNEL * (256-1e-5)) / (1 << (ARC_ENCODE_D_BITS - 8)), (1 << (ARC_ENCODE_Y_BITS - 8)))) +
		   v.ARC_ENCODE_Y_CHANNEL) / (1 << (ARC_ENCODE_Y_BITS - 8));
	float d = v.ARC_ENCODE_D_CHANNEL + float (mod (int (v.ARC_ENCODE_OTHER_CHANNEL * (256-1e-5)), (1 << (ARC_ENCODE_D_BITS - 8)))) / (1 << 8);
	d = MAX_D * (2 * d - 1);
	return vec3 (x, y, d);
      }
      
      vec3 arc_decodek (const vec4 v)
      {
	float x = (float ((mod (int (v.a * 256) / (1 << (ARC_ENCODE_D_BITS - 8 + ARC_ENCODE_Y_BITS - 8)), (1 << (ARC_ENCODE_X_BITS - 8)))) * (1<< 8)) + v.r) / (1 << (ARC_ENCODE_X_BITS - 1));
	float y = (float ((mod (int (v.a * 256) / (1 << (ARC_ENCODE_D_BITS - 8)), (1 << (ARC_ENCODE_Y_BITS - 8)))) * (1<< 8)) + v.g) / (1 << (ARC_ENCODE_Y_BITS - 1));
	float d = (float ((mod (int (v.a * 256), (1 << (ARC_ENCODE_D_BITS - 8)))) * (1<< 8)) + v.b) / (1 << (ARC_ENCODE_D_BITS - 1));
	d = MAX_D * (2 * d - 1);
	return vec3 (x, y, d);
      }

      vec2 rgba_t_to_pair (const vec4 v)
      {
        float x = (int (256 * v.r) * 256 + int (256 * v.g));
        float y = (int (256 * v.b) * 256 + int (256 * v.a));
        return vec2 (x, y);
      }

      /*void main1()
      {
	float m = float (fwidth (p)); // isotropic antialiasing 
	int i;
	float min_dist = 1;
	float min_point_dist = 1;
	vec3 arc_next = arc_decode (texture2D (tex, vec2(.5,.5 / float(num_points))));
	
	for (i = 0; i < num_points - 1; i++) {
	  vec3 arc = arc_next;
	  arc_next = arc_decode (texture2D (tex, vec2(.5, (1.5 + float(i)) / float(num_points))));
	  float d = arc.b;
	  if (d == -MAX_D) continue;
	  if (abs (d) < 1e-5) d = 1e-5; // cheat 
	  vec2 p0 = arc.rg;
	  vec2 p1 = arc_next.rg;
	  vec2 line = p1 - p0;
	  vec2 perp = perpendicular (line);
	  vec2 norm = normalize (perp);
	  vec2 c = mix (p0, p1, .5) - perp * ((1 - d*d) / (4 * d));

	  float dist;
	  if (sign (d) * dot (p - c, perpendicular (p0 - c)) <= 0 &&
	      sign (d) * dot (p - c, perpendicular (p1 - c)) >= 0)
	  {
	    dist = abs (distance (p, c) - distance (p0, c));
	  } else {
	    dist = min (distance (p, p0), distance (p, p1));
	  }
	  min_dist = min (min_dist, dist);

	  float point_dist = min (distance (p, p0), distance (p, p1));
	  min_point_dist = min (min_point_dist, point_dist);
	}

	gl_FragColor = mix(vec4(1,0,0,1),
			   vec4(1,1,1,1) * ((1 + sin (min_dist / m)) / 2) * sin (pow (min_dist, .8) * 3.14159265358979),
			   smoothstep (0, 2 * m, min_dist));
	gl_FragColor = mix(vec4(0,1,0,1),
			   gl_FragColor,
			   smoothstep (.002, .005, min_point_dist));
      } 
      */
      
      void main()
      {
	float m = float (fwidth (p)); /* isotropic antialiasing */
		
	int p_cell_x = int (p.x * GRIDSIZE);
	int p_cell_y = int (p.y * GRIDSIZE);

	
	vec2 arc_position_data = rgba_t_to_pair(texture2D (tex, vec2(0.5, float(.5 + (p_cell_y * GRIDSIZE + p_cell_x)) / float(texture_size))));
	int offset = int(arc_position_data.x);
	int num_endpoints =  int(arc_position_data.y);
	
	
	
	int i;
	float min_dist = 1.;
	float min_point_dist = 1.;
	vec3 arc_next = arc_decode (texture2D (tex, vec2(.5, (.5 + float(offset)) / float(texture_size))));
	
	
	for (i = 0; i < min(num_endpoints - 1, 786) ; i++) {  
	// I don't understand 
	// How or why the min there helps. 
	// Yet somehow it works.
	
	  vec3 arc = arc_next;
	  arc_next = arc_decode (texture2D (tex, vec2(.5, (1.5 + float(i) + float(offset)) / float(texture_size))));
	  float d = arc.b;
	  if (d == -MAX_D) continue;
	  if (abs (d) < 1e-5) d = 1e-5; // cheat 
	  vec2 p0 = arc.rg;
	  vec2 p1 = arc_next.rg;
	  vec2 line = p1 - p0;
	  vec2 perp = perpendicular (line);
	  vec2 norm = normalize (perp);
	  vec2 c = mix (p0, p1, .5) - perp * ((1 - d*d) / (4 * d));

	  float dist;
	  if (sign (d) * dot (p - c, perpendicular (p0 - c)) <= 0 &&
	      sign (d) * dot (p - c, perpendicular (p1 - c)) >= 0)
	  {
	    dist = abs (distance (p, c) - distance (p0, c));
	  } else {
	    dist = min (distance (p, p0), distance (p, p1));
	  }
	  min_dist = min (min_dist, dist);

	  float point_dist = min (distance (p, p0), distance (p, p1));
	  min_point_dist = min (min_point_dist, point_dist);
	}
	
	gl_FragColor = mix(vec4(1,0,0,1),
			   vec4(1,1,1,1) * ((1 + sin (min_dist / m)) / 2) * sin (pow (min_dist, .8) * 3.14159265358979),
			   smoothstep (0, 2 * m, min_dist));
	gl_FragColor = mix(vec4(0,1,0,1),
			   gl_FragColor,
			   smoothstep (.002, .005, min_point_dist));
	return;
    }
  );
  program = create_program (vshader, fshader);

  glUseProgram (program);
  glUniform1i (glGetUniformLocation(program, "tex"), 0);
  glActiveTexture (GL_TEXTURE0);

  glGenTextures (1, &texture);
  glBindTexture (GL_TEXTURE_2D, texture);

  setup_texture (font_path, utf8, program);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  a_pos_loc = glGetAttribLocation(program, "a_position");
  a_tex_loc = glGetAttribLocation(program, "a_texCoord");

  glVertexAttribPointer(a_pos_loc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), w_vertices+0);
  glVertexAttribPointer(a_tex_loc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), w_vertices+3);

  glEnableVertexAttribArray(a_pos_loc);
  glEnableVertexAttribArray(a_tex_loc);

  gtk_widget_set_double_buffered (window, FALSE);
  gtk_widget_set_redraw_on_allocate (window, TRUE);
  g_signal_connect (G_OBJECT (window), "expose-event", G_CALLBACK (expose_cb), GINT_TO_POINTER (glGetUniformLocation (program, "u_matViewProjection")));
  g_signal_connect (G_OBJECT (window), "configure-event", G_CALLBACK (configure_cb), NULL);

  if (animate) {
    g_idle_add (step, window->window);
    g_timeout_add (5000, print_fps, NULL);
  }

  gtk_main ();

  return 0;
}
