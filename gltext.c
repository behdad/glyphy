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
#define COMPILE_SHADER(Type,Src) compile_shader (Type, #Src)

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

static void
setup_texture (void)
{
#define FONTSIZE 256
#define FONTFAMILY "serif"
#define TEXT "ab"
  int width = 0, height = 0;
  cairo_surface_t *image = NULL;
  cairo_t *cr;
  int i;

  for (i = 0; i < 2; i++) {
    cairo_text_extents_t extents;

    if (image)
      cairo_surface_destroy (image);

    image = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    cr = cairo_create (image);
    cairo_set_source_rgb (cr, 1., 1., 1.);
    cairo_paint (cr);
    cairo_set_source_rgb (cr, 0., 0., 0.);

    cairo_select_font_face (cr, FONTFAMILY, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, FONTSIZE);
    cairo_text_extents (cr, TEXT, &extents);
    width = ceil (extents.x_bearing + extents.width) - floor (extents.x_bearing);
    height = ceil (extents.y_bearing + extents.height) - floor (extents.y_bearing);
    cairo_move_to (cr, -floor (extents.x_bearing), -floor (extents.y_bearing));
    cairo_show_text (cr, TEXT);
    cairo_destroy (cr);
  }


  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, cairo_image_surface_get_data (image));
  cairo_surface_write_to_png (image, "glyph.png");

  cairo_surface_destroy (image);
}


GLuint texture;
static
gboolean expose_cb (GtkWidget *widget,
		    GdkEventExpose *event,
		    gpointer user_data)
{
  GtkAllocation allocation;
  static int i = 0;
  double theta = M_PI / 360.0 * i;
  GLfloat mat[] = { +cos(theta), +sin(theta), 0., 0.,
		    -sin(theta), +cos(theta), 0., 0.,
			     0.,          0., 1., 0.,
			     0.,          0., 0., 1., };

  drawable_make_current (widget->window);

  glBindTexture (GL_TEXTURE_2D, texture);
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
  GLuint vshader, fshader, program, a_pos_loc, a_tex_loc;
  const GLfloat w_vertices[] = { -0.50, -0.50, +0.00,
				 +1.00, +0.00,
				 +0.50, -0.50, +0.00,
				 +0.00, +0.00,
				 +0.50, +0.50, +0.00,
				 +0.00, +1.00,
				 -0.50, +0.50, +0.00,
				 +1.00, +1.00 };


  gtk_init (&argc, &argv);

  window = GTK_WIDGET (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  gtk_window_set_default_size (GTK_WINDOW (window), 500, 500);
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
	gl_FragColor = texture2D(tex, v_texCoord);
	gl_FragColor.a = .5;
      }
  );
  program = create_program (vshader, fshader);

  glUseProgram(program);
  glUniform1i(glGetUniformLocation(program, "tex"), 0);
  glActiveTexture (GL_TEXTURE0);

  glGenTextures (1, &texture);
  glBindTexture (GL_TEXTURE_2D, texture);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  setup_texture ();

  a_pos_loc = glGetAttribLocation(program, "a_position");
  a_tex_loc = glGetAttribLocation(program, "a_texCoord");

  glVertexAttribPointer(a_pos_loc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), w_vertices);
  glVertexAttribPointer(a_tex_loc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &w_vertices[3]);

  glEnableVertexAttribArray(a_pos_loc);
  glEnableVertexAttribArray(a_tex_loc);

  gtk_widget_set_app_paintable (window, TRUE);
  gtk_widget_set_double_buffered (window, FALSE);
  gtk_widget_set_redraw_on_allocate (window, TRUE);
  g_signal_connect (G_OBJECT (window), "expose-event", G_CALLBACK (expose_cb), GINT_TO_POINTER (glGetUniformLocation (program, "u_matViewProjection")));

  g_timeout_add (1000 / 60, step, window->window);

  gtk_main ();

  return 0;
}
