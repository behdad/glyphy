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
setup_texture (void)
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




/** Given a point, finds the shortest distance to the arc that is closest to that point. 
  * Sign of the distance depends on whether point is "inside" or "outside" the glyph.
  * (Negative distance corresponds to being inside the glyph.)
  */
static double
distance_to_an_arc (Point<Coord> p,
                    vector <Arc <Coord, Scalar> > arc_list)
{
  double min_distance = INFINITY;
  int nearest_arc = 0;

  for (int k = 0; k < arc_list.size (); k++)  {
    double current_distance = arc_list.at(k).distance_to_point (p);    

    /* If two arcs are equally close to this point, take the sign from the one
       whose extension is farther away. (Extend arcs using tangent lines from endpoints;
       this is done using the SignedVector operation "-".) */
    if (fabs (fabs (current_distance) - fabs(min_distance)) < 1e-6) { 
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


/***************************************************************************************************************************/
/*** This uses the new SDF approach. ***************************************************************************************/
/***************************************************************************************************************************/
static void
setup_texture (void)
{
#define TEXSIZE 64
#define SAMPLING 4
#define UTF8 'G'
#define FILTERWIDTH 8
  int width = 0, height = 0, swidth, sheight;
  cairo_surface_t *image = NULL, *dest;

  unsigned char *data;



  FT_Face face;
  FT_Library library;
  FT_Init_FreeType (&library);
   
//  FT_New_Face ( library, "./googlefontdirectory/eatercaps/EaterCaps-Regular.ttf", 0, &face ); 
  FT_New_Face ( library, "./googlefontdirectory/ultra/Ultra.ttf", 0, &face ); 

  unsigned int upem = face->units_per_EM ;
  FT_Set_Char_Size (face, upem*64, upem*64, 0, 0);


  double tolerance = upem * 1e-5; //1e-3; /* in font design units */



  width = (*face).max_advance_width / 1.5;
  height = (*face).height / 1.5;

  swidth = width;
  sheight = height;

  width = (width + SAMPLING-1) / SAMPLING + 2 * FILTERWIDTH;
  height = (height + SAMPLING-1) / SAMPLING + 2 * FILTERWIDTH;
  width = (width+3)&~3;;


  /************************************************** NEW CODE STARTS HERE. ****************************************************/

  printf("Width: %d. Height: %d. SWidth: %d. SHeight: %d.\n", width, height, swidth, sheight);



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


  acc.arcs.clear ();
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





/************************************************************************************************************************8888888*****************/

  
  // SDF stuff to change.
  {
    unsigned char *dd;
    int x, y;
  //  data = cairo_image_surface_get_data (image);


    dest = cairo_image_surface_create (CAIRO_FORMAT_A8, width, height);
    dd = cairo_image_surface_get_data (dest);

  
    for (y = 0; y < height; y++)
      for (x = 0; x < width; x++) {
   //     printf("Started! (x, y) = (%d, %d).\n", x, y);
        int sx, sy, i, j;
        double c;



  /************************************************** NEW CODE STARTS HERE. ****************************************************/

  // Main goal: Set D(x,y) = { 0   if inside, distance >= FW * SMP
  //                           127 if inside, distance == 0
  //                           255 if outside, distance >= FW * SMP

#define S(x,y) ((x)<0||(y)<0||(x)>=swidth||(y)>=sheight ? 0 : data[(y)*swidth+(x)])
#define D(x,y) (dd[(y)*width+(x)])

  sx = (width - FILTERWIDTH - (x - FILTERWIDTH)) * SAMPLING;	// For some reason, horizontally flipped by default. Quick fix.
  sy = (y - FILTERWIDTH) * SAMPLING;
  double d = distance_to_an_arc (Point<Coord> (sx, sy), acc.arcs) ;

  if (d <= -1 * FILTERWIDTH * SAMPLING)
    D(x,y) = 255;
  else if (d >= FILTERWIDTH * SAMPLING)
    D(x,y) = 0;
  else
    D(x,y) = d * (-127.5) / (FILTERWIDTH * SAMPLING) + 127.5;




//  printf ("(%d, %d). Our d = %g. Our D = %d.   ", sx, sy, d, D(x,y));
   } 


    data = cairo_image_surface_get_data (dest);
  }


 // return;

  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
//  cairo_surface_write_to_png (image, "glyph.png");



 // cairo_surface_destroy (image); 
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
  glClearColor(0., 1., 0., 0.);
  glClear(GL_COLOR_BUFFER_BIT);

  glUniformMatrix4fv (GPOINTER_TO_INT (user_data), 1, GL_FALSE, mat);

  glDrawArrays (GL_TRIANGLE_FAN, 0, 4);

  drawable_swap_buffers (widget->window);

  i++;

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
  GLuint vshader, fshader, program, texture, a_pos_loc, a_tex_loc;
#define ZOOM 2
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
  gtk_window_set_default_size (GTK_WINDOW (window), 300, 300);
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
	float mm = m * 128. / (FILTERWIDTH*SAMPLING);

	float alpha = smoothstep (-mm, mm, texture2D(tex, v_texCoord).r - .5);
	vec4 c;

	//alpha = texture2D(tex, v_texCoord);
	if (v_texCoord.s < 5)
	  c = mix (vec4(0,0,0,1), vec4(1,1,1,1), alpha);
	else
	  c = mix (vec4(1,1,1,1), vec4(0,0,0,1), alpha);
	//c = sqrt (c);
	gl_FragColor = c;
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

  setup_texture ();

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
