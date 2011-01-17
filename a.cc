#include <X11/Xlib.h>
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
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    fprintf(stderr, "Shader compile error\n");
    GLint info_len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 0) {
      char *info_log = (char*) malloc (info_len);
      glGetShaderInfoLog (shader, info_len, NULL, info_log);

      fprintf (stderr, "%s\n", info_log);
      free (info_log);
    }
  }

  return shader;
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

egl_drawable_t *
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

void
drawable_make_current (GdkDrawable *drawable)
{
  egl_drawable_t *e = drawable_get_egl (drawable);
  eglMakeCurrent(e->display, e->surface, e->surface, e->context);
}

void
drawable_swap_buffers (GdkDrawable *drawable)
{
  egl_drawable_t *e = drawable_get_egl (drawable);
  eglSwapBuffers (e->display, e->surface);
}

int
main(int argc, char** argv)
{
  GtkWidget *window;

  gtk_init (&argc, &argv);

  window = (GtkWidget *) gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  Display* dpy = gdk_x11_display_get_xdisplay (gtk_widget_get_display (window));
  EGLDisplay edpy = eglGetDisplay(dpy);
  EGLint major, minor;
  eglInitialize(edpy, &major, &minor);

  if (!eglBindAPI(EGL_OPENGL_ES_API)) {
    fprintf(stderr, "failed to bind EGL_OPENGL_ES_API\n");
    return 0;
  }

  gtk_widget_show_all (window);

  XID xwindow;
  xwindow = GDK_WINDOW_XID (window->window);

  int width = 300, height = 300;
  GdkDrawable *pixmap;
  {

    printf("Egl %d.%d\n", major, minor);

    pixmap = gdk_pixmap_new (window->window, width, height, -1);

    drawable_make_current (pixmap);

    GLuint p_vert_shader = compile_shader (GL_VERTEX_SHADER,
	"attribute vec4 vPosition;"
	"uniform mat4 u_matViewProjection;"
	"void main()"
	"{"
	"   gl_Position = u_matViewProjection * vPosition;"
	"}");

    GLuint p_frag_shader = compile_shader(GL_FRAGMENT_SHADER,
	"precision mediump float;"
	"void main()"
	"{"
	"  gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);"
	"}");

    GLuint p_program = glCreateProgram();
    glAttachShader(p_program, p_vert_shader);
    glAttachShader(p_program, p_frag_shader);
    glLinkProgram(p_program);

    glUseProgram(p_program);
    GLfloat p_vertices[] = { +0.50, +0.00, +0.00,
			   -0.25, +0.43, +0.00,
			   -0.25, -0.43, +0.00 };
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, p_vertices);
    glEnableVertexAttribArray(0);

    GLint p_mat_loc = glGetUniformLocation(p_program, "u_matViewProjection");

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
    drawable_make_current (window->window);

    GLuint w_vert_shader = compile_shader (GL_VERTEX_SHADER,
        "attribute vec4 a_position;"
        "attribute vec2 a_texCoord;"
        "uniform mat4 u_matViewProjection;"
        "varying vec2 v_texCoord;"
        "void main()"
        "{"
        "  gl_Position = u_matViewProjection * a_position;"
        "  v_texCoord = a_texCoord;"
        "}");

    GLuint w_frag_shader = compile_shader (GL_FRAGMENT_SHADER,
        "uniform sampler2D tex;"
        "varying vec2 v_texCoord;"
        "void main()"
        "{"
        "  gl_FragColor = texture2D(tex, v_texCoord);"
        "}");

    GLuint w_program = glCreateProgram();
    glAttachShader(w_program, w_vert_shader);
    glAttachShader(w_program, w_frag_shader);
    glLinkProgram(w_program);

    // Texture from pixmap.
    EGLImageKHR i_pixmap = eglCreateImageKHR(
        edpy, drawable_get_egl (window->window)->context, EGL_NATIVE_PIXMAP_KHR, (void*) GDK_PIXMAP_XID (pixmap), NULL);
    GLuint pixmap_tex;
    glGenTextures(1, &pixmap_tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pixmap_tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, i_pixmap);

    // Bind to program.
    glUseProgram(w_program);
    GLint w_tex_loc = glGetUniformLocation(w_program, "tex");
    glUniform1i(w_tex_loc, 0);

    GLint w_mat_loc = glGetUniformLocation(w_program, "u_matViewProjection");

    GLfloat w_vertices[] = { -0.50, -0.50, +0.00,
                             +1.00, +0.00,
                             +0.50, -0.50, +0.00,
                             +0.00, +0.00,
                             +0.50, +0.50, +0.00,
                             +0.00, +1.00,
                             -0.50, +0.50, +0.00,
                             +1.00, +1.00 };

    GLuint w_a_pos_loc = glGetAttribLocation(w_program, "a_position");
    GLuint w_a_tex_loc = glGetAttribLocation(w_program, "a_texCoord");

    glVertexAttribPointer(w_a_pos_loc, 3, GL_FLOAT, 
                          GL_FALSE, 5 * sizeof(GLfloat), w_vertices);
    glVertexAttribPointer(w_a_tex_loc, 2, GL_FLOAT,
                          GL_FALSE, 5 * sizeof(GLfloat), &w_vertices[3]);

    glEnableVertexAttribArray(w_a_pos_loc);
    glEnableVertexAttribArray(w_a_tex_loc);

    for(int i=0; i<10000; i++) {
      double theta = M_PI / 360.0 * i;
      GLfloat mat[] = { +cos(theta), +sin(theta), 0., 0.,
                        -sin(theta), +cos(theta), 0., 0.,
                                 0.,          0., 1., 0.,
                                 0.,          0., 0., 1., };

      glViewport(0, 0, width, height);
      glClearColor(0., 0., 0., 0.);
      glClear(GL_COLOR_BUFFER_BIT);

      glUniformMatrix4fv(w_mat_loc, 1, GL_FALSE, mat);

      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

      drawable_swap_buffers (window->window);
    }
  }

  gtk_main ();
}
