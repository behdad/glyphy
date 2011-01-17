#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <GLES2/gl2.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

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

  if (!(shader = glCreateShader(type)))
    return shader;

  glShaderSource (shader, 1, &source, 0);
  glCompileShader (shader);

  GLint compiled;
  glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    fprintf(stderr, "Shader compile error\n");
    GLint info_len = 0;
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
  GLuint program = glCreateProgram();
  glAttachShader(program, vshader);
  glAttachShader(program, fshader);
  glLinkProgram(program);

  GLint linked;
  glGetProgramiv (program, GL_LINK_STATUS, &linked);
  if (!linked) {
    fprintf(stderr, "Program linking error\n");
    GLint info_len = 0;
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

  if (!eglChooseConfig(edpy, attribs, &econfig, 1, &num_configs) || !num_configs)
    die ("Could not find EGL config");

  if (!(*surface = eglCreateWindowSurface (edpy, econfig, GDK_DRAWABLE_XID (drawable), NULL)))
    die ("Could not create EGL surface");

  EGLint ctx_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  if (!(*context = eglCreateContext (edpy, econfig, EGL_NO_CONTEXT, ctx_attribs)))
    die ("Could not create EGL context");
}

GQuark
egl_drawable_quark (void)
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
} egl_drawable_t;

static void
egl_drawable_destroy (egl_drawable_t *e)
{
  eglDestroyContext (e->display, e->context);
  eglDestroySurface (e->display, e->surface);
  g_slice_free (egl_drawable_t, e);
}

static egl_drawable_t *
drawable_get_egl (GdkDrawable *drawable)
{
  egl_drawable_t *e;

  if (G_UNLIKELY (!(e = (egl_drawable_t *) g_object_get_qdata ((GObject *) drawable, egl_drawable_quark ())))) {
    e = g_slice_new (egl_drawable_t);
    e->display = eglGetDisplay (GDK_DRAWABLE_XDISPLAY (drawable));
    create_egl_for_drawable (e->display, drawable, &e->surface, &e->context);
    g_object_set_qdata_full (G_OBJECT (drawable), egl_drawable_quark (), e, (GDestroyNotify) egl_drawable_destroy);
  }

  return e;
}

static void
drawable_make_current (GdkDrawable *drawable)
{
  egl_drawable_t *e = drawable_get_egl (drawable);
  eglMakeCurrent(e->display, e->surface, e->surface, e->context);
}

static void
drawable_swap_buffers (GdkDrawable *drawable)
{
  egl_drawable_t *e = drawable_get_egl (drawable);
  eglSwapBuffers (e->display, e->surface);
}

static EGLImageKHR
pixmap_create_texture_image (GdkPixmap *pixmap)
{
  return eglCreateImageKHR (eglGetCurrentDisplay (),
			    eglGetCurrentContext (),
			    EGL_NATIVE_PIXMAP_KHR,
			    (EGLClientBuffer) GDK_PIXMAP_XID (pixmap),
			    NULL);
}


static
gboolean expose_cb (GtkWidget *widget,
		    GdkEventExpose *event,
		    gpointer user_data)
{
  GtkAllocation allocation;
  static int i = 0;

  i++;

  gtk_widget_get_allocation (widget, &allocation);

  drawable_make_current (widget->window);

  double theta = M_PI / 360.0 * i;
  GLfloat mat[] = { +cos(theta), +sin(theta), 0., 0.,
		    -sin(theta), +cos(theta), 0., 0.,
			     0.,          0., 1., 0.,
			     0.,          0., 0., 1., };

  glViewport(0, 0, allocation.width, allocation.height);
  glClearColor(0., 0., 0., 0.);
  glClear(GL_COLOR_BUFFER_BIT);

  glUniformMatrix4fv (GPOINTER_TO_INT (user_data), 1, GL_FALSE, mat);

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  drawable_swap_buffers (widget->window);

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
  GdkDrawable *pixmap;
  GtkWidget *window;

  gtk_init (&argc, &argv);

  window = GTK_WIDGET (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  gtk_window_set_default_size (GTK_WINDOW (window), 500, 500);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  EGLDisplay edpy = eglGetDisplay (gdk_x11_display_get_xdisplay (gtk_widget_get_display (window)));
  EGLint major, minor;
  eglInitialize (edpy, &major, &minor);
  if (!eglBindAPI (EGL_OPENGL_ES_API))
    die ("Failed to bind OpenGL ES API");

  gtk_widget_show_all (window);

  int width = 300, height = 300;
  pixmap = gdk_pixmap_new (window->window, width, height, -1);

  {
    GLuint vshader, fshader, program;

    drawable_make_current (pixmap);

    vshader = COMPILE_SHADER (GL_VERTEX_SHADER,
	attribute vec4 vPosition;
	uniform mat4 u_matViewProjection;
	void main()
	{
	   gl_Position = u_matViewProjection * vPosition;
	}
    );
    fshader = COMPILE_SHADER (GL_FRAGMENT_SHADER,
	precision mediump float;
	void main()
	{
	  gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);
	}
    );
    program = create_program (vshader, fshader);

    glUseProgram(program);
    GLfloat p_vertices[] = { +0.50, +0.00, +0.00,
			   -0.25, +0.43, +0.00,
			   -0.25, -0.43, +0.00 };
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, p_vertices);
    glEnableVertexAttribArray(0);

    GLint p_mat_loc = glGetUniformLocation (program, "u_matViewProjection");

    {
      glViewport(0, 0, width, height);
      glClearColor(0.25, 0.25, 0.25, 0.);
      glClear(GL_COLOR_BUFFER_BIT);

      double theta = 0 * M_PI / 360.0;
      GLfloat mat[] = { +cos(theta), +sin(theta), 0., 0.,
			-sin(theta), +cos(theta), 0., 0.,
				 0.,          0., 1., 0.,
				0.,          0., 0., 1., };
      glUniformMatrix4fv(p_mat_loc, 1, GL_FALSE, mat);

      glDrawArrays(GL_TRIANGLES, 0, 3);
    }
  }
  {
    GLuint vshader, fshader, program;
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
        }
    );
    program = create_program (vshader, fshader);


    EGLImageKHR image = pixmap_create_texture_image (pixmap);
    GLuint pixmap_tex;
    glGenTextures(1, &pixmap_tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pixmap_tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

    // Bind to program.
    glUseProgram(program);
    GLint w_tex_loc = glGetUniformLocation(program, "tex");
    glUniform1i(w_tex_loc, 0);

    GLint w_mat_loc = glGetUniformLocation(program, "u_matViewProjection");

    GLfloat w_vertices[] = { -0.50, -0.50, +0.00,
                             +1.00, +0.00,
                             +0.50, -0.50, +0.00,
                             +0.00, +0.00,
                             +0.50, +0.50, +0.00,
                             +0.00, +1.00,
                             -0.50, +0.50, +0.00,
                             +1.00, +1.00 };

    GLuint w_a_pos_loc = glGetAttribLocation(program, "a_position");
    GLuint w_a_tex_loc = glGetAttribLocation(program, "a_texCoord");

    glVertexAttribPointer(w_a_pos_loc, 3, GL_FLOAT, 
                          GL_FALSE, 5 * sizeof(GLfloat), w_vertices);
    glVertexAttribPointer(w_a_tex_loc, 2, GL_FLOAT,
                          GL_FALSE, 5 * sizeof(GLfloat), &w_vertices[3]);

    glEnableVertexAttribArray(w_a_pos_loc);
    glEnableVertexAttribArray(w_a_tex_loc);

    gtk_widget_set_app_paintable (window, TRUE);
    gtk_widget_set_double_buffered (window, FALSE);
    gtk_widget_set_redraw_on_allocate (window, TRUE);
    g_signal_connect (G_OBJECT (window), "expose-event", G_CALLBACK (expose_cb), GINT_TO_POINTER (w_mat_loc));
  }

 g_timeout_add (1000 / 60, step, window->window);

  gtk_main ();
}
