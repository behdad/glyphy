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
static const int NUM_SAVED_ARCS = 20;

struct grid_cell {
  char arcs [NUM_SAVED_ARCS]; 
  char more_arcs_pointer;
  bool inside_glyph;
};

#define TEXSIZE 132
struct arcs_texture  /********************************************************************************* Struct or Class? **************************/
{  
    std::vector<point_t> arc_endpoints;
    std::vector<double>  d_values;    
    grid_cell grid [TEXSIZE][TEXSIZE];  /********************************************************* 32 by 32? 64 by 64? ***************************/
};


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
}

/***************************************************************************************************************************/
/*** This is the OLD way of doing it. Artifacts on edges, rounded corners. *************************************************/
/**************************************************************************************************************************
static void
setup_texture (char* hi)
{
#define TEXSIZE 64
#define SAMPLING 8
  //unsigned char data[height][width][4];
#define FONTSIZE (TEXSIZE * SAMPLING)
#define FONTFAMILY "serif"
#define TEXT "a"
#define FILTERWIDTH 8
  int width = 0, height = 0, swidth, sheight;
  cairo_surface_t *image = NULL, *dest;
  cairo_t *cr;
  unsigned char *data;
  int i;

  for (i = 0; i < 2; i++) {
    cairo_text_extents_t extents;

    if (image)
      cairo_surface_destroy (image);

    image = cairo_image_surface_create (CAIRO_FORMAT_A8, width, height);
    cr = cairo_create (image);
    cairo_set_source_rgb (cr, 0., 0., 0.);

    cairo_select_font_face (cr, FONTFAMILY, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, FONTSIZE);
    cairo_text_extents (cr, TEXT, &extents);
    width = ceil (extents.x_bearing + extents.width) - floor (extents.x_bearing);
    height = ceil (extents.y_bearing + extents.height) - floor (extents.y_bearing);
    width = (width+3)&~3;;
    cairo_move_to (cr, -floor (extents.x_bearing), -floor (extents.y_bearing));
    cairo_show_text (cr, TEXT);
    cairo_destroy (cr);
  }

  swidth = width;
  sheight = height;

  width = (width + SAMPLING-1) / SAMPLING + 2 * FILTERWIDTH;
  height = (height + SAMPLING-1) / SAMPLING + 2 * FILTERWIDTH;
  width = (width+3)&~3;;
  {
    unsigned char *dd;
    int x, y;
    data = cairo_image_surface_get_data (image);
    dest = cairo_image_surface_create (CAIRO_FORMAT_A8, width, height);
    dd = cairo_image_surface_get_data (dest);
    for (y = 0; y < height; y++)
      for (x = 0; x < width; x++) {
        int sx, sy, i, j;
        double c;

	sx = (x - FILTERWIDTH) * SAMPLING;
	sy = (y - FILTERWIDTH) * SAMPLING;

        c =  1e10;
#define S(x,y) ((x)<0||(y)<0||(x)>=swidth||(y)>=sheight ? 0 : data[(y)*swidth+(x)])
#define D(x,y) (dd[(y)*width+(x)])
	if (S(sx,sy) >= 128) {
	  // in //
	  for (i = -FILTERWIDTH*SAMPLING; i <= FILTERWIDTH*SAMPLING; i++)
	    for (j = -FILTERWIDTH*SAMPLING; j <= FILTERWIDTH*SAMPLING; j++)
	      if (S(sx+i,sy+j) < 128)
		c = MIN (c, sqrt (i*i + j*j));
	  D(x,y) = 128 - MIN (c, FILTERWIDTH*SAMPLING) * 128. / (FILTERWIDTH*SAMPLING);
	} else {
	  // out //
	  for (i = -FILTERWIDTH*SAMPLING; i <= FILTERWIDTH*SAMPLING; i++)
	    for (j = -FILTERWIDTH*SAMPLING; j <= FILTERWIDTH*SAMPLING; j++)
	      if (S(sx+i,sy+j) >= 128)
		c = MIN (c, sqrt (i*i + j*j));
	  D(x,y) = 127 + MIN (c, FILTERWIDTH*SAMPLING) * 128. / (FILTERWIDTH*SAMPLING);
	}
      }
    data = cairo_image_surface_get_data (dest);
  }

  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
  cairo_surface_write_to_png (image, "glyph.png");

  cairo_surface_destroy (image);
}
/***************************************************************************************************************************/



/* Given a list of arcs, finds a reasonable set of boundaries for a grid that covers all of them. */
static void
find_grid_boundaries (double &grid_min_x, 
                      double &grid_max_x,
                      double &grid_min_y, 
                      double &grid_max_y, 
                      vector <Arc <Coord, Scalar> > arc_list)
{
  grid_min_x = INFINITY; //-20;
  grid_max_x = -1 * INFINITY; //350;
  grid_min_y = INFINITY; //-120;
  grid_max_y = -1 * INFINITY; //210;

  for (int i = 0; i < arc_list.size (); i++)  {
    Arc<Coord, Scalar> arc = arc_list.at(i);
    grid_min_x = std::min (grid_min_x, (fabs(arc.d) < 0.3 ? std::min (arc.p0.x, arc.p1.x) : arc.center().x - arc.radius()));
    grid_max_x = std::max (grid_max_x, (fabs(arc.d) < 0.3 ? std::max (arc.p0.x, arc.p1.x) : arc.center().x + arc.radius()));
    grid_min_y = std::min (grid_min_y, (fabs(arc.d) < 0.3 ? std::min (arc.p0.y, arc.p1.y) : arc.center().y - arc.radius()));
    grid_max_y = std::max (grid_max_y, (fabs(arc.d) < 0.3 ? std::max (arc.p0.y, arc.p1.y) : arc.center().y + arc.radius()));
  }
  grid_min_x *= 0.98;
  grid_min_y *= 0.98;
  grid_max_y *= 1.02;
  grid_max_x *= 1.02;
}

/** Given a point, finds the shortest distance to the arc that is closest to that point. 
  * Sign of the distance depends on whether point is "inside" or "outside" the glyph.
  * (Negative distance corresponds to being inside the glyph.)
  */
static double
distance_to_an_arc (point_t p, arcs_texture tex, int x, int y)
{
  arc_t nearest_arc (tex.arc_endpoints.at (0),
                         tex.arc_endpoints.at (1),
                         tex.d_values.at (0));
  double min_distance = (tex.grid[x][y].inside_glyph ? -1 : 1) * INFINITY; //nearest_arc.distance_to_point (p);
  int arc_index = tex.grid[x][y].arcs[0];

  for (int k = 0; k < NUM_SAVED_ARCS && arc_index >= 0; k++)  { 
    if (tex.d_values.at (arc_index) < INFINITY) {         
      arc_t current_arc (tex.arc_endpoints.at (arc_index),
                         tex.arc_endpoints.at (arc_index + 1),
                         tex.d_values.at (arc_index));
                         
      double current_distance = current_arc.distance_to_point (p);    

    /* If two arcs are equally close to this point, take the sign from the one
       whose extension is farther away. (Extend arcs using tangent lines from endpoints;
       this is done using the SignedVector operation "-".) */
      if (fabs (fabs (current_distance) - fabs(min_distance)) < 1e-6) { 
        SignedVector<Coord> to_arc_min = nearest_arc - p;
        SignedVector<Coord> to_arc_current = current_arc - p;
      
        if (to_arc_min.len () < to_arc_current.len ()) {
          min_distance = fabs (min_distance) * (to_arc_current.negative ? -1 : 1);
        }
      }
      else if (fabs (current_distance) < fabs(min_distance)) {
        min_distance = current_distance;
        nearest_arc = current_arc;
      }
    }
      
    arc_index = tex.grid[x][y].arcs[k + 1];
  }
  return min_distance;
}

/** Given a cell, fills the vector closest_arcs with arcs that may be closest to some point in the cell.
  * Uses idea that all close arcs to cell must be ~close to center of cell. 
  */
static void 
closest_arcs_to_cell (Point<Coord> square_top_left, 
                        Scalar cell_width, 
                        Scalar cell_height,
                        Scalar grid_size,
                        arcs_texture tex,
                        grid_cell &cell)
{
  cell.inside_glyph = false;
  // Initialize array of arcs with non-arcs.
  for (int i = 0; i < NUM_SAVED_ARCS; i++)
    cell.arcs[i] = -1;
    

  /* Find distance between cell center and cell's closest arc. */
  point_t center (square_top_left.x + cell_width / 2., 
                       square_top_left.y + cell_height / 2.);
  double min_distance = INFINITY;
  double distance = min_distance;

  for (int k = 0; k < tex.arc_endpoints.size (); k++) {
    if (tex.d_values.at (k) != INFINITY) {
      arc_t current_arc (tex.arc_endpoints.at (k),
                           tex.arc_endpoints.at (k+1),
                           tex.d_values.at (k));
      distance = fabs (current_arc.squared_distance_to_point (center));
      if (distance < min_distance) {
        min_distance = distance;
        cell.inside_glyph = (current_arc - center).negative ? true : false; 
      }
    }
  }
  
    
  /* If d is the distance from the center of the square to the nearest arc, then
     all nearest arcs to the square must be at most [d + s/sqrt(2)] from the center. */
  min_distance = sqrt (min_distance);
  double half_diagonal = sqrt(cell_height * cell_height + cell_width * cell_width) / 2;
  Scalar radius = min_distance + half_diagonal;
  printf("Minimum distance is %g. Winning Distances: ", min_distance);

  double tolerance = grid_size / min_font_size; 
 
  int array_index = 0;
 // if (min_distance - half_diagonal <= tolerance) 
    for (int k = 0; k < tex.arc_endpoints.size (); k++) {
      if (tex.d_values.at (k) != INFINITY) {
        arc_t current_arc (tex.arc_endpoints.at (k),
                           tex.arc_endpoints.at (k+1),
                           tex.d_values.at (k));
        if (fabs(current_arc.distance_to_point (center)) < radius) {
          printf("%g (%d), ", current_arc.distance_to_point (center), k);
          cell.arcs[array_index] = k;      
          array_index++;
        }      
      }
    }
    
    printf("\n  Array of indices:");
    
    
  for (int k = 0; k < NUM_SAVED_ARCS; k++)
    printf("%d ", cell.arcs[k]);
  printf("\n");
}




/***************************************************************************************************************************/
/*** This uses the new SDF approach. ***************************************************************************************/
/***************************************************************************************************************************/
static void
setup_texture (const char *font_path)
{
#define UTF8 's'

  int width = 0, height = 0;
  cairo_surface_t *image = NULL, *dest;
  unsigned char *data;  
  FT_Face face;
  FT_Library library;
  FT_Init_FreeType (&library);   
  FT_New_Face ( library, font_path, 0, &face );
  unsigned int upem = face->units_per_EM;
  double tolerance = upem * 1e-3; // in font design units 

  /************************************************************************************** These dimensions are not /quite/ perfect... *********************/
  width = TEXSIZE; //(*face).max_advance_width / 1.5;
  height = TEXSIZE; //(*face).height / 1.5;
  width = (width+3)&~3;;
  FT_Set_Char_Size (face, TEXSIZE, TEXSIZE, 0, 0);
  printf("Width: %d. Height: %d.\n", width, height);


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
  

  arcs_texture tex;

  acc.arcs.clear ();
  tex.arc_endpoints.clear ();
  tex.d_values.clear ();
  
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
	  
  double grid_min_x, grid_max_x, grid_min_y, grid_max_y, glyph_width, glyph_height;
  find_grid_boundaries (grid_min_x, grid_max_x, grid_min_y, grid_max_y, acc.arcs);
  
  glyph_width = grid_max_x - grid_min_x;
  glyph_height = grid_max_y - grid_min_y;
  printf ("Glyph dimensions: width = %g, height = %g.\n", glyph_width, glyph_height);

  // Populate lists for storing arc data.
    /**************************************************************************************************************************************************/
  int arc_count;
  for (arc_count = 0; arc_count < acc.arcs.size () - 1; arc_count++) {
    arc_t current_arc = acc.arcs.at (arc_count);
    tex.arc_endpoints.push_back (current_arc.p0);
    tex.d_values.push_back (current_arc.d);
    // Close the current loop in the outline.
    if (current_arc.p1 != acc.arcs.at (arc_count+1).p0) {
      tex.arc_endpoints.push_back (current_arc.p1);
      tex.d_values.push_back (INFINITY);
    }
  }
  // The last arc needs to be done specially.
  tex.arc_endpoints.push_back (acc.arcs.at (arc_count).p0);
  tex.arc_endpoints.push_back (acc.arcs.at (arc_count).p1);
  tex.d_values.push_back (acc.arcs.at (arc_count).d);
  tex.d_values.push_back (INFINITY);
  
  
  // Display the arc data.
  #if 1 
  // Print out the lists we will use, namely point and d-value data.
  printf ("Our List:\n");
  for (int i = 0; i < tex.arc_endpoints.size(); i++)
    printf ("  %d.\t(%g, %g), d = %g.\n", i, tex.arc_endpoints.at (i).x, tex.arc_endpoints.at (i).y, tex.d_values.at(i));
    
  // Print out the original list for comparison, sorted by arc rather than point.
  printf ("Correct List:\n");
  for (int i = 0; i < acc.arcs.size (); i++)	
    printf("  %d.\td = %g. Have (%g,%g) to (%g,%g).\n", i, acc.arcs.at (i).d,
    				 acc.arcs.at (i).p0.x,
    				 acc.arcs.at (i).p0.y,
    				 acc.arcs.at (i).p1.x,
    				 acc.arcs.at (i).p1.y);

  #endif


  // Make a 2d grid for arc/cell information.
  /**************************************************************************************************************************************************/
  
  double box_width = glyph_width / TEXSIZE;
  double box_height = glyph_height / TEXSIZE;
//  grid_max_y += 1 * box_height;    
//  grid_min_y += 1 * box_height;

  
  double min_dimension = std::min(glyph_width, glyph_height);
  for (int row = 0; row < TEXSIZE; row++) 
    for (int col = 0; col < TEXSIZE; col++) 
      closest_arcs_to_cell (Point<Coord> (grid_min_x + (col * box_width), grid_min_y + (row * box_height)), 
                              box_width, box_height, min_dimension, 
                              tex, tex.grid[col][row]);        



  // SDF stuff to change.
  /**************************************************************************************************************************************************/
  
  {
    unsigned char *dd;
    int x, y;

    dest = cairo_image_surface_create (CAIRO_FORMAT_A8, width, height);
    dd = cairo_image_surface_get_data (dest);
    
#define D(x,y) (dd[(y)*width+(x)])  
// Main goal: Set D(x,y) = { 0   if inside, distance >= TEXSIZE
//                           127 if inside, distance == 0
//                           255 if outside, distance <= -TEXSIZE

printf("Starting loop.... ");

    printf("We scale x by %g and y by %g ...", glyph_width / (TEXSIZE), glyph_height /(TEXSIZE));

    for (y = 0; y < height; y++)
      for (x = 0; x < width; x++) {
        double d = distance_to_an_arc (Point<Coord> (grid_min_x + (x * box_width),
        					     grid_min_y + (y * box_height)),
        					     tex, x, y) / 1; /***************************** Increasing denominator decreases contrast. *********/     
	y = height - y - 1;
	
        // Antialiasing (?) happens here (?). 
        if (d <= -1 * TEXSIZE) 
          D(x, y) = 255;
        else if (d >= TEXSIZE) 
          D(x, y) = 0;
        else
          D(x, y) = d * (-127.5) / TEXSIZE + 127.5; 
          
        y = height - y - 1;
      } 

    data = cairo_image_surface_get_data (dest);
  }

  printf("Nearly done.... ");
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
  printf("Done setup.\n");
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

static
gboolean expose_cb (GtkWidget *widget,
		    GdkEventExpose *event,
		    gpointer user_data)
{
  GtkAllocation allocation;
  static int i = 0;
  double theta = M_PI / 360.0 * i / 3.;
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

  // Comment out to disable rotation.
  // i++;

  return TRUE;
}

static gboolean
step (gpointer data)
{
  gdk_window_invalidate_rect (GDK_WINDOW (data), NULL, TRUE);
  return TRUE;
}

int
main (int argc, char** argv)
{
  GtkWidget *window;
  char *font_path;
  if (argc >= 1) {
     font_path = argv[1];
  }
  
  
  GLuint vshader, fshader, program, texture, a_pos_loc, a_tex_loc;
  
  // Draw a grid of (ZOOM x ZOOM) glyphs.
  // TODO: Figure out what this list means, and how!!!!!!!!!!!!!!!!!!!!!!
#define ZOOM 1
  const GLfloat w_vertices[] = { -1.00, -1.00, +0.00,
				 +0.00, ZOOM,
				 +1.00, -1.00, +0.00,
				 ZOOM, ZOOM,
				 +1.00, +1.00, +0.00,
				 ZOOM, +0.00,
				 -1.00, +1.00, +0.00,
				 +0.00, +0.00 };


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
      varying vec2 v_texCoord;
      void main()
      {
	gl_Position = u_matViewProjection * a_position;
	v_texCoord = a_texCoord;
      }
  );
  fshader = COMPILE_SHADER (GL_FRAGMENT_SHADER,
      uniform sampler2D tex;
      varying vec2 v_texCoord;
      void main()
      {
	float ddx = length (dFdx (v_texCoord));
	float ddy = length (dFdy (v_texCoord));
	float m = max (ddx, ddy); /* isotropic antialiasing */
	float mm = m * 128. / 32 ;/// (TEXSIZE); //FILTERWIDTH*SAMPLING);

	float alpha = smoothstep (-mm, mm, texture2D(tex, v_texCoord).r - .5);
	float alpha2 = texture2D(tex, v_texCoord).r;

	//alpha = texture2D(tex, v_texCoord);
	//vec4 c = mix (vec4(0,0,0,1), vec4(1,1,1,1), alpha2);
	vec4 c2 = mix (vec4(0,0,1,1), vec4(0,1,0,1), alpha2);
	vec4 c_real = mix (vec4(0,0,0,1), vec4(1,1,1,1), alpha);
	//c = sqrt (c);
	gl_FragColor = mix(c_real, c2, .7); //c_real;
      }
  );
  program = create_program (vshader, fshader);

  glUseProgram (program);
  glUniform1i (glGetUniformLocation(program, "tex"), 0);
  glActiveTexture (GL_TEXTURE0);

  glGenTextures (1, &texture);
  glBindTexture (GL_TEXTURE_2D, texture);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  //glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  //glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  setup_texture (font_path);

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

  g_timeout_add (1000 / 60, step, window->window);

  gtk_main ();

  return 0;
}
